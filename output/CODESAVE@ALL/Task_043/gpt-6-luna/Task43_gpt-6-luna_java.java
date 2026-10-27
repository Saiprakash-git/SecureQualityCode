import com.sun.net.httpserver.HttpExchange;
import com.sun.net.httpserver.HttpServer;

import java.io.IOException;
import java.io.OutputStream;
import java.net.InetSocketAddress;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.security.SecureRandom;
import java.util.Base64;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;
import java.util.regex.Pattern;

public final class SessionWebApp {
    private static final long SESSION_TTL_MILLIS = TimeUnit.MINUTES.toMillis(30);
    private static final SecureRandom RANDOM = new SecureRandom();
    private static final Pattern SESSION_ID_PATTERN = Pattern.compile("[A-Za-z0-9_-]{43}");

    private final Map<String, Session> sessions = new ConcurrentHashMap<>();
    private final boolean secureCookies;
    private final String cookieName;

    private SessionWebApp(boolean secureCookies) {
        this.secureCookies = secureCookies;
        this.cookieName = secureCookies ? "__Host-SESSION" : "SESSIONID";
    }

    public static void main(String[] args) throws Exception {
        boolean secureCookies = true;
        for (String arg : args) {
            if ("--insecure-cookie".equals(arg)) {
                secureCookies = false;
            }
        }

        int port = configuredPort();
        SessionWebApp app = new SessionWebApp(secureCookies);
        HttpServer server = HttpServer.create(new InetSocketAddress(port), 128);
        server.createContext("/", app::handle);
        server.setExecutor(Executors.newFixedThreadPool(
                Math.max(4, Runtime.getRuntime().availableProcessors())));
        ScheduledExecutorService cleanup = Executors.newSingleThreadScheduledExecutor(r -> {
            Thread thread = new Thread(r, "session-cleanup");
            thread.setDaemon(true);
            return thread;
        });
        cleanup.scheduleAtFixedRate(app::removeExpiredSessions, 1, 1, TimeUnit.MINUTES);
        Runtime.getRuntime().addShutdownHook(new Thread(() -> {
            server.stop(1);
            cleanup.shutdownNow();
        }));
        server.start();
        System.out.println("Session web application listening on port " + port);
    }

    private static int configuredPort() {
        String configured = System.getenv("PORT");
        if (configured == null || configured.isBlank()) {
            return 8080;
        }
        try {
            int port = Integer.parseInt(configured);
            if (port < 1 || port > 65535) {
                throw new IllegalArgumentException("PORT must be between 1 and 65535");
            }
            return port;
        } catch (NumberFormatException exception) {
            throw new IllegalArgumentException("PORT must be a valid integer", exception);
        }
    }

    private void handle(HttpExchange exchange) throws IOException {
        try {
            addSecurityHeaders(exchange);
            String path = exchange.getRequestURI().getPath();
            String method = exchange.getRequestMethod();

            if ("/health".equals(path)) {
                if ("GET".equals(method)) {
                    sendJson(exchange, 200, "{\"status\":\"ok\"}");
                } else {
                    methodNotAllowed(exchange, "GET");
                }
                return;
            }

            if ("/".equals(path)) {
                if ("GET".equals(method)) {
                    sendJson(exchange, 200,
                            "{\"service\":\"session-manager\",\"endpoints\":"
                                    + "{\"create\":\"POST /session\","
                                    + "\"inspect\":\"GET /session\","
                                    + "\"rotate\":\"POST /session/rotate\","
                                    + "\"terminate\":\"POST /session/logout or DELETE /session\"}}");
                } else {
                    methodNotAllowed(exchange, "GET");
                }
                return;
            }

            if ("/session".equals(path)) {
                handleSession(exchange, method);
                return;
            }

            if ("/session/rotate".equals(path)) {
                if ("POST".equals(method)) {
                    rotateSession(exchange);
                } else {
                    methodNotAllowed(exchange, "POST");
                }
                return;
            }

            if ("/session/logout".equals(path)) {
                if ("POST".equals(method)) {
                    terminateSession(exchange);
                } else {
                    methodNotAllowed(exchange, "POST");
                }
                return;
            }

            sendJson(exchange, 404, "{\"error\":\"not_found\"}");
        } catch (IOException exception) {
            throw exception;
        } catch (Exception exception) {
            if (exchange.getResponseCode() == -1) {
                sendJson(exchange, 500, "{\"error\":\"internal_error\"}");
            }
        } finally {
            exchange.close();
        }
    }

    private void handleSession(HttpExchange exchange, String method) throws IOException {
        if ("POST".equals(method)) {
            createSession(exchange);
        } else if ("GET".equals(method)) {
            inspectSession(exchange);
        } else if ("DELETE".equals(method)) {
            terminateSession(exchange);
        } else {
            methodNotAllowed(exchange, "GET, POST, DELETE");
        }
    }

    private void createSession(HttpExchange exchange) throws IOException {
        SessionRef existing = findSession(exchange);
        if (existing != null) {
            sessions.remove(existing.id, existing.session);
        }

        String id = randomToken();
        Session session = new Session();
        sessions.put(id, session);
        setSessionCookie(exchange, id, false);
        sendSessionJson(exchange, 201, session);
    }

    private void inspectSession(HttpExchange exchange) throws IOException {
        SessionRef ref = findSession(exchange);
        if (ref == null) {
            sendJson(exchange, 401, "{\"error\":\"session_required\"}");
            return;
        }
        sendSessionJson(exchange, 200, ref.session);
    }

    private void rotateSession(HttpExchange exchange) throws IOException {
        SessionRef ref = findSession(exchange);
        if (ref == null) {
            sendJson(exchange, 401, "{\"error\":\"session_required\"}");
            return;
        }
        if (!hasValidCsrfToken(exchange, ref.session)) {
            sendJson(exchange, 403, "{\"error\":\"csrf_validation_failed\"}");
            return;
        }

        Session replacement = new Session();
        synchronized (ref.session) {
            if (ref.session.isExpired(System.currentTimeMillis())
                    || !sessions.remove(ref.id, ref.session)) {
                clearSessionCookie(exchange);
                sendJson(exchange, 401, "{\"error\":\"session_expired\"}");
                return;
            }
            sessions.put(randomToken(), replacement);
        }

        String replacementId = findIdForSession(replacement);
        if (replacementId == null) {
            sendJson(exchange, 500, "{\"error\":\"internal_error\"}");
            return;
        }
        setSessionCookie(exchange, replacementId, false);
        sendSessionJson(exchange, 200, replacement);
    }

    private String findIdForSession(Session session) {
        for (Map.Entry<String, Session> entry : sessions.entrySet()) {
            if (entry.getValue() == session) {
                return entry.getKey();
            }
        }
        return null;
    }

    private void terminateSession(HttpExchange exchange) throws IOException {
        SessionRef ref = findSession(exchange);
        if (ref == null) {
            clearSessionCookie(exchange);
            sendNoContent(exchange);
            return;
        }
        if (!hasValidCsrfToken(exchange, ref.session)) {
            sendJson(exchange, 403, "{\"error\":\"csrf_validation_failed\"}");
            return;
        }

        synchronized (ref.session) {
            sessions.remove(ref.id, ref.session);
        }
        clearSessionCookie(exchange);
        sendNoContent(exchange);
    }

    private SessionRef findSession(HttpExchange exchange) {
        String id = readSessionCookie(exchange);
        if (id == null || !SESSION_ID_PATTERN.matcher(id).matches()) {
            return null;
        }

        Session session = sessions.get(id);
        if (session == null) {
            return null;
        }

        synchronized (session) {
            long now = System.currentTimeMillis();
            if (session.isExpired(now)) {
                sessions.remove(id, session);
                return null;
            }
            session.lastAccessedAt = now;
            session.expiresAt = now + SESSION_TTL_MILLIS;
        }
        return new SessionRef(id, session);
    }

    private boolean hasValidCsrfToken(HttpExchange exchange, Session session) {
        String supplied = exchange.getRequestHeaders().getFirst("X-CSRF-Token");
        if (supplied == null || supplied.length() > 128) {
            return false;
        }
        synchronized (session) {
            return MessageDigest.isEqual(
                    session.csrfToken.getBytes(StandardCharsets.UTF_8),
                    supplied.getBytes(StandardCharsets.UTF_8));
        }
    }

    private String readSessionCookie(HttpExchange exchange) {
        String found = null;
        for (String header : exchange.getRequestHeaders().getOrDefault("Cookie", java.util.List.of())) {
            for (String part : header.split(";")) {
                String item = part.trim();
                int separator = item.index