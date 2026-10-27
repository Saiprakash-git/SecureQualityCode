import com.sun.net.httpserver.Headers;
import com.sun.net.httpserver.HttpExchange;
import com.sun.net.httpserver.HttpServer;

import java.io.IOException;
import java.net.InetSocketAddress;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashSet;
import java.util.Locale;
import java.util.Set;

public final class CorsApiServer {
    private static final String API_PATH = "/api/message";
    private static final Set<String> ALLOWED_METHODS =
            Collections.unmodifiableSet(new HashSet<>(Arrays.asList("GET")));
    private static final Set<String> ALLOWED_HEADERS =
            Collections.unmodifiableSet(new HashSet<>(Arrays.asList("content-type", "authorization")));

    private final Set<String> allowedOrigins;

    public CorsApiServer(Set<String> allowedOrigins) {
        this.allowedOrigins = Collections.unmodifiableSet(new HashSet<>(allowedOrigins));
    }

    public static void main(String[] args) throws IOException {
        int port = args.length > 0 ? Integer.parseInt(args[0]) : 8080;
        Set<String> origins = args.length > 1
                ? new HashSet<>(Arrays.asList(args[1].split(",")))
                : new HashSet<>(Arrays.asList("http://localhost:3000", "http://127.0.0.1:3000"));

        CorsApiServer application = new CorsApiServer(origins);
        HttpServer server = HttpServer.create(new InetSocketAddress(port), 0);
        server.createContext("/", application::handleRequest);
        server.start();
        System.out.println("CORS API listening on port " + port);
    }

    private void handleRequest(HttpExchange exchange) throws IOException {
        Headers responseHeaders = exchange.getResponseHeaders();
        addVaryOrigin(responseHeaders);

        String origin = exchange.getRequestHeaders().getFirst("Origin");
        if (origin != null && !allowedOrigins.contains(origin)) {
            sendJson(exchange, 403, "{\"error\":\"Origin not allowed\"}");
            return;
        }

        if (origin != null) {
            responseHeaders.set("Access-Control-Allow-Origin", origin);
        }

        if (!API_PATH.equals(exchange.getRequestURI().getPath())) {
            sendJson(exchange, 404, "{\"error\":\"Not found\"}");
            return;
        }

        if ("OPTIONS".equalsIgnoreCase(exchange.getRequestMethod())) {
            handlePreflight(exchange, origin);
            return;
        }

        if (!"GET".equalsIgnoreCase(exchange.getRequestMethod())) {
            responseHeaders.set("Allow", "GET, OPTIONS");
            sendJson(exchange, 405, "{\"error\":\"Method not allowed\"}");
            return;
        }

        sendJson(exchange, 200, "{\"message\":\"Hello from the CORS-enabled API\"}");
    }

    private void handlePreflight(HttpExchange exchange, String origin) throws IOException {
        Headers requestHeaders = exchange.getRequestHeaders();
        String requestedMethod = requestHeaders.getFirst("Access-Control-Request-Method");

        if (origin == null || requestedMethod == null
                || !ALLOWED_METHODS.contains(requestedMethod.toUpperCase(Locale.ROOT))) {
            sendJson(exchange, 403, "{\"error\":\"Preflight request not allowed\"}");
            return;
        }

        String requestedHeaders = requestHeaders.getFirst("Access-Control-Request-Headers");
        if (requestedHeaders != null && !areHeadersAllowed(requestedHeaders)) {
            sendJson(exchange, 403, "{\"error\":\"Requested headers not allowed\"}");
            return;
        }

        Headers responseHeaders = exchange.getResponseHeaders();
        responseHeaders.set("Access-Control-Allow-Methods", "GET");
        if (requestedHeaders != null && !requestedHeaders.trim().isEmpty()) {
            responseHeaders.set("Access-Control-Allow-Headers", requestedHeaders);
        }
        responseHeaders.set("Access-Control-Max-Age", "600");
        exchange.sendResponseHeaders(204, -1);
        exchange.close();
    }

    private boolean areHeadersAllowed(String requestedHeaders) {
        for (String header : requestedHeaders.split(",")) {
            String normalizedHeader = header.trim().toLowerCase(Locale.ROOT);
            if (normalizedHeader.isEmpty() || !ALLOWED_HEADERS.contains(normalizedHeader)) {
                return false;
            }
        }
        return true;
    }

    private void addVaryOrigin(Headers headers) {
        String currentValue = headers.getFirst("Vary");
        if (currentValue == null || currentValue.isEmpty()) {
            headers.set("Vary", "Origin");
        } else if (!currentValue.toLowerCase(Locale.ROOT).contains("origin")) {
            headers.set("Vary", currentValue + ", Origin");
        }
    }

    private void sendJson(HttpExchange exchange, int status, String body) throws IOException {
        byte[] response = body.getBytes(StandardCharsets.UTF_8);
        exchange.getResponseHeaders().set("Content-Type", "application/json; charset=utf-8");
        exchange.getResponseHeaders().set("X-Content-Type-Options", "nosniff");
        exchange.sendResponseHeaders(status, response.length);
        exchange.getResponseBody().write(response);
        exchange.close();
    }
}