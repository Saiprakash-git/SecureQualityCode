import com.sun.net.httpserver.HttpServer;
import com.sun.net.httpserver.HttpHandler;
import com.sun.net.httpserver.HttpExchange;
import java.io.*;
import java.net.InetSocketAddress;
import java.net.URLDecoder;
import java.security.SecureRandom;
import java.util.*;
import java.util.concurrent.*;

public class SecureSessionManager {
    private static final int PORT = 8080;
    private static final long SESSION_TIMEOUT_MS = 30 * 60 * 1000;
    private static final String SESSION_COOKIE_NAME = "SESSION_ID";
    private final Map<String, Session> sessions = new ConcurrentHashMap<>();
    private final SecureRandom secureRandom = new SecureRandom();
    private final ScheduledExecutorService scheduler = Executors.newScheduledThreadPool(1);

    static class Session {
        final String id;
        final String username;
        final long createdAt;
        volatile long lastAccessedAt;
        final Map<String, String> attributes = new ConcurrentHashMap<>();

        Session(String id, String username) {
            this.id = id;
            this.username = username;
            this.createdAt = System.currentTimeMillis();
            this.lastAccessedAt = this.createdAt;
        }

        boolean isExpired() {
            return System.currentTimeMillis() - lastAccessedAt > SESSION_TIMEOUT_MS;
        }

        void touch() {
            this.lastAccessedAt = System.currentTimeMillis();
        }
    }

    public SecureSessionManager() {
        scheduler.scheduleAtFixedRate(this::cleanupExpiredSessions, 1, 1, TimeUnit.MINUTES);
    }

    private void cleanupExpiredSessions() {
        sessions.entrySet().removeIf(entry -> entry.getValue().isExpired());
    }

    String generateSessionId() {
        byte[] randomBytes = new byte[32];
        secureRandom.nextBytes(randomBytes);
        StringBuilder sb = new StringBuilder();
        for (byte b : randomBytes) {
            sb.append(String.format("%02x", b));
        }
        return sb.toString();
    }

    Session createSession(String username) {
        String sessionId = generateSessionId();
        Session session = new Session(sessionId, username);
        sessions.put(sessionId, session);
        return session;
    }

    Session getSession(String sessionId) {
        if (sessionId == null) return null;
        Session session = sessions.get(sessionId);
        if (session != null && !session.isExpired()) {
            session.touch();
            return session;
        } else if (session != null) {
            sessions.remove(sessionId);
        }
        return null;
    }

    void destroySession(String sessionId) {
        if (sessionId != null) {
            sessions.remove(sessionId);
        }
    }

    String getSessionIdFromCookie(HttpExchange exchange) {
        String cookieHeader = exchange.getRequestHeaders().getFirst("Cookie");
        if (cookieHeader != null) {
            for (String cookie : cookieHeader.split(";")) {
                String[] parts = cookie.trim().split("=", 2);
                if (parts.length == 2 && parts[0].equals(SESSION_COOKIE_NAME)) {
                    return parts[1];
                }
            }
        }
        return null;
    }

    void setSessionCookie(HttpExchange exchange, String sessionId) {
        String cookie = String.format("%s=%s; Path=/; HttpOnly; Secure; SameSite=Strict; Max-Age=%d",
            SESSION_COOKIE_NAME, sessionId, SESSION_TIMEOUT_MS / 1000);
        exchange.getResponseHeaders().add("Set-Cookie", cookie);
    }

    void clearSessionCookie(HttpExchange exchange) {
        String cookie = String.format("%s=; Path=/; HttpOnly; Secure; SameSite=Strict; Max-Age=0",
            SESSION_COOKIE_NAME);
        exchange.getResponseHeaders().add("Set-Cookie", cookie);
    }

    void sendResponse(HttpExchange exchange, int statusCode, String response) throws IOException {
        byte[] bytes = response.getBytes("UTF-8");
        exchange.getResponseHeaders().set("Content-Type", "text/html; charset=UTF-8");
        exchange.sendResponseHeaders(statusCode, bytes.length);
        try (OutputStream os = exchange.getResponseBody()) {
            os.write(bytes);
        }
    }

    class LoginHandler implements HttpHandler {
        @Override
        public void handle(HttpExchange exchange) throws IOException {
            if (!"POST".equals(exchange.getRequestMethod())) {
                sendResponse(exchange, 405, "<h1>Method Not Allowed</h1>");
                return;
            }

            String body = new String(exchange.getRequestBody().readAllBytes(), "UTF-8");
            Map<String, String> params = parseFormData(body);

            String username = params.get("username");
            String password = params.get("password");

            if (username != null && password != null && authenticate(username, password)) {
                Session session = createSession(username);
                setSessionCookie(exchange, session.id);
                sendResponse(exchange, 200, "<h1>Login Successful</h1><p>Welcome " + escapeHtml(username) + "</p><a href=\"/dashboard\">Dashboard</a> | <a href=\"/logout\">Logout</a>");
            } else {
                sendResponse(exchange, 401, "<h1>Login Failed</h1><p>Invalid credentials</p><a href=\"/login\">Try again</a>");
            }
        }
    }

    class LogoutHandler implements HttpHandler {
        @Override
        public void handle(HttpExchange exchange) throws IOException {
            String sessionId = getSessionIdFromCookie(exchange);
            if (sessionId != null) {
                destroySession(sessionId);
            }
            clearSessionCookie(exchange);
            sendResponse(exchange, 200, "<h1>Logged Out</h1><p>Session terminated securely.</p><a href=\"/login\">Login</a>");
        }
    }

    class DashboardHandler implements HttpHandler {
        @Override
        public void handle(HttpExchange exchange) throws IOException {
            String sessionId = getSessionIdFromCookie(exchange);
            Session session = getSession(sessionId);

            if (session == null) {
                exchange.getResponseHeaders().set("Location", "/login");
                exchange.sendResponseHeaders(302, -1);
                return;
            }

            String html = "<h1>Dashboard</h1>" +
                         "<p>Welcome, " + escapeHtml(session.username) + "</p>" +
                         "<p>Session ID: " + session.id.substring(0, 8) + "...</p>" +
                         "<p>Created: " + new Date(session.createdAt) + "</p>" +
                         "<p>Last Access: " + new Date(session.lastAccessedAt) + "</p>" +
                         "<