import com.sun.net.httpserver.HttpExchange;
import com.sun.net.httpserver.HttpHandler;
import com.sun.net.httpserver.HttpServer;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.InetSocketAddress;
import java.net.URLDecoder;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;
import java.time.Duration;
import java.time.Instant;
import java.util.Base64;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;
import java.util.Optional;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

public class SecureSessionApplication {

    public static final String SESSION_COOKIE_NAME = "__Host-SessionId";
    private static final int PORT = 8443;

    public static void main(String[] args) throws IOException {
        Duration idleTimeout = Duration.ofMinutes(15);
        Duration absoluteTimeout = Duration.ofHours(2);
        SessionManager sessionManager = new SessionManager(idleTimeout, absoluteTimeout);

        UserManager userManager = new UserManager();
        userManager.registerUser("admin", "SuperSecretPassw0rd!");
        userManager.registerUser("alice", "AliceSecurePassw0rd#2026");

        HttpServer server = HttpServer.create(new InetSocketAddress(PORT), 0);
        server.setExecutor(Executors.newVirtualThreadPerTaskExecutor());

        server.createContext("/api/login", new LoginHandler(sessionManager, userManager));
        server.createContext("/api/logout", new LogoutHandler(sessionManager));
        server.createContext("/api/profile", new ProfileHandler(sessionManager));
        server.createContext("/api/status", new StatusHandler(sessionManager));

        server.start();
        System.out.println("Secure session server started on port " + PORT);
    }

    public static final class Session {
        private final String id;
        private final String userId;
        private final Instant createdAt;
        private volatile Instant lastAccessedAt;
        private final Map<String, Object> attributes;

        public Session(String id, String userId, Instant createdAt) {
            this.id = id;
            this.userId = userId;
            this.createdAt = createdAt;
            this.lastAccessedAt = createdAt;
            this.attributes = new ConcurrentHashMap<>();
        }

        public String getId() {
            return id;
        }

        public String getUserId() {
            return userId;
        }

        public Instant getCreatedAt() {
            return createdAt;
        }

        public Instant getLastAccessedAt() {
            return lastAccessedAt;
        }

        public void touch(Instant now) {
            this.lastAccessedAt = now;
        }

        public void setAttribute(String key, Object value) {
            this.attributes.put(key, value);
        }

        public Optional<Object> getAttribute(String key) {
            return Optional.ofNullable(this.attributes.get(key));
        }

        public Map<String, Object> getAttributes() {
            return Collections.unmodifiableMap(this.attributes);
        }
    }

    public static final class SessionManager {
        private final Map<String, Session> sessions = new ConcurrentHashMap<>();
        private final SecureRandom secureRandom = new SecureRandom();
        private final Duration idleTimeout;
        private final Duration absoluteTimeout;
        private final ScheduledExecutorService reaperService = Executors.newSingleThreadScheduledExecutor();

        public SessionManager(Duration idleTimeout, Duration absoluteTimeout) {
            this.idleTimeout = idleTimeout;
            this.absoluteTimeout = absoluteTimeout;
            this.reaperService.scheduleAtFixedRate(this::purgeExpiredSessions, 1, 1, TimeUnit.MINUTES);
        }

        public Session createSession(String userId) {
            String sessionId = generateSecureToken();
            Session session = new Session(sessionId, userId, Instant.now());
            sessions.put(sessionId, session);
            return session;
        }

        public Optional<Session> getValidSession(String sessionId) {
            if (sessionId == null || sessionId.isBlank()) {
                return Optional.empty();
            }

            Session session = sessions.get(sessionId);
            if (session == null) {
                return Optional.empty();
            }

            Instant now = Instant.now();
            if (isExpired(session, now)) {
                invalidateSession(sessionId);
                return Optional.empty();
            }

            session.touch(now);
            return Optional.of(session);
        }

        public boolean invalidateSession(String sessionId) {
            if (sessionId == null) {
                return false;
            }
            return sessions.remove(sessionId) != null;
        }

        public int purgeExpiredSessions() {
            Instant now = Instant.now();
            int purgedCount = 0;
            for (Map.Entry<String, Session> entry : sessions.entrySet()) {
                if (isExpired(entry.getValue(), now)) {
                    if (sessions.remove(entry.getKey(), entry.getValue())) {
                        purgedCount++;
                    }
                }
            }
            return purgedCount;
        }

        public int getActiveSessionCount() {
            return sessions.size();
        }

        public Duration getIdleTimeout() {
            return idleTimeout;
        }

        private boolean isExpired(Session session, Instant now) {
            boolean idleExpired = Duration.between(session.getLastAccessedAt(), now).compareTo(idleTimeout) > 0;
            boolean absoluteExpired = Duration.between(session.getCreatedAt(), now).compareTo(absoluteTimeout) > 0;
            return idleExpired || absoluteExpired;
        }

        private String generateSecureToken() {
            byte[] randomBytes = new byte[32];
            this.secureRandom.nextBytes(randomBytes);
            return Base64.getUrlEncoder().withoutPadding().encodeToString(randomBytes);
        }

        public void shutdown() {
            reaperService.shutdown();
        }
    }

    public static final class UserManager {
        private final Map<String, UserRecord> users = new ConcurrentHashMap<>();
        private final SecureRandom secureRandom = new SecureRandom();

        private record UserRecord(String username, byte[] salt, byte[] passwordHash) {}

        public boolean registerUser(String username, String rawPassword) {
            if (users.containsKey(username)) {
                return false;
            }
            byte[] salt = new byte[16];
            secureRandom.nextBytes(salt);
            byte[] hash = hashPassword(rawPassword, salt);
            users.put(username, new UserRecord(username, salt, hash));
            return true;
        }

        public boolean authenticate(String username, String rawPassword) {
            UserRecord record = users.get(username);
            if (record == null) {
                return false;
            }
            byte[] computedHash = hashPassword(rawPassword, record.salt());
            return MessageDigest.isEqual(record.passwordHash(), computedHash);
        }

        private byte[] hashPassword(String password, byte[] salt) {
            try {
                MessageDigest digest = MessageDigest.getInstance("SHA-256");
                digest.update(salt);
                return digest.digest(password.getBytes(StandardCharsets.UTF_8));
            } catch (NoSuchAlgorithmException e) {
                throw new RuntimeException("SHA-256 algorithm not available", e);
            }
        }
    }

    private static class LoginHandler implements HttpHandler {
        private final SessionManager sessionManager;
        private final UserManager userManager;

        public LoginHandler(SessionManager sessionManager, UserManager userManager) {
            this.sessionManager = sessionManager;
            this.userManager = userManager;
        }

        @Override
        public void handle(HttpExchange exchange) throws IOException {
            if (!"POST".equalsIgnoreCase(exchange.getRequestMethod())) {
                sendJsonResponse(exchange, 405, "{\"error\":\"Method Not Allowed\"}");
                return;
            }

            Map<String, String> bodyParams = parseRequestBody(exchange.getRequestBody());
            String username = bodyParams.get("username");
            String password = bodyParams.get("password");

            if (username == null || password == null || !userManager.authenticate(username, password)) {
                sendJsonResponse(exchange, 401, "{\"error\":\"Invalid username or password\"}");
                return;
            }

            Optional<String> existingSessionId = extractCookieValue(exchange, SESSION_COOKIE_NAME);
            existingSessionId.ifPresent(sessionManager::invalidateSession);

            Session newSession = sessionManager.createSession(username);
            newSession.setAttribute("ipAddress", exchange.getRemoteAddress().getAddress().getHostAddress());
            newSession.setAttribute("userAgent", exchange.getRequestHeaders().getFirst("User-Agent"));

            String cookieHeader = buildSessionCookie(newSession.getId(), (int) sessionManager.getIdleTimeout().toSeconds());
            exchange.getResponseHeaders().add("Set-Cookie", cookieHeader);

            String response = "{\"status\":\"success\",\"message\":\"Authenticated successfully\",\"user\":\"" + username + "\"}";
            sendJsonResponse(exchange, 200, response);
        }
    }

    private static class LogoutHandler implements HttpHandler {
        private final SessionManager sessionManager;

        public LogoutHandler(SessionManager sessionManager) {
            this.sessionManager = sessionManager;
        }

        @Override
        public void handle(HttpExchange exchange) throws IOException {
            if (!"POST".equalsIgnoreCase(exchange.getRequestMethod())) {
                sendJsonResponse(exchange, 405, "{\"error\":\"Method Not Allowed\"}");
                return;
            }

            Optional<String> sessionId = extractCookieValue(exchange, SESSION_COOKIE_NAME);
            sessionId.ifPresent(sessionManager::invalidateSession);

            String deletionCookie = buildSessionCookie("", 0);
            exchange.getResponseHeaders().add("Set-Cookie", deletionCookie);

            String response = "{\"status\":\"success\",\"message\":\"Logged out successfully\"}";
            sendJsonResponse(exchange, 200, response);
        }
    }

    private static class ProfileHandler implements HttpHandler {
        private final SessionManager sessionManager;

        public ProfileHandler(SessionManager sessionManager) {
            this.sessionManager = sessionManager;
        }

        @Override
        public void handle(HttpExchange exchange) throws IOException {
            if (!"GET".equalsIgnoreCase(exchange.getRequestMethod())) {
                sendJsonResponse(exchange, 405, "{\"error\":\"Method Not Allowed\"}");
                return;
            }

            Optional<String> sessionId = extractCookieValue(exchange, SESSION_COOKIE_NAME);
            if (sessionId.isEmpty()) {
                sendJsonResponse(exchange, 401, "{\"error\":\"Authentication required\"}");
                return;
            }

            Optional<Session> session = sessionManager.getValidSession(sessionId.get());
            if (session.isEmpty()) {
                String deletionCookie = buildSessionCookie("", 0);
                exchange.getResponseHeaders().add("Set-Cookie", deletionCookie);
                sendJsonResponse(exchange, 401, "{\"error\":\"Session expired or invalid\"}");
                return;
            }

            Session activeSession = session.get();
            String response = String.format(
                    "{\"user\":\"%s\",\"createdAt\":\"%s\",\"lastAccessedAt\":\"%s\"}",
                    activeSession.getUserId(),
                    activeSession.getCreatedAt(),
                    activeSession.getLastAccessedAt()
            );
            sendJsonResponse(exchange, 200, response);
        }
    }

    private static class StatusHandler implements HttpHandler {
        private final SessionManager sessionManager;

        public StatusHandler(SessionManager sessionManager) {
            this.sessionManager = sessionManager;
        }

        @Override
        public void handle(HttpExchange exchange) throws IOException {
            if (!"GET".equalsIgnoreCase(exchange.getRequestMethod())) {
                sendJsonResponse(exchange, 40