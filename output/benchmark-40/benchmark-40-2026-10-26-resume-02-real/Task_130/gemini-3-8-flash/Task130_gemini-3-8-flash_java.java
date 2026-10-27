import com.sun.net.httpserver.Headers;
import com.sun.net.httpserver.HttpExchange;
import com.sun.net.httpserver.HttpHandler;
import com.sun.net.httpserver.HttpServer;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.InetSocketAddress;
import java.net.URI;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashSet;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Locale;
import java.util.Set;
import java.util.concurrent.Executors;

public class CorsApiServer {

    public static class CorsConfiguration {
        private final Set<String> allowedOrigins = new HashSet<>();
        private final Set<String> allowedMethods = new HashSet<>();
        private final Set<String> allowedHeaders = new HashSet<>();
        private final Set<String> exposedHeaders = new HashSet<>();
        private boolean allowCredentials = false;
        private long maxAgeSeconds = 1800;
        private boolean allowAllOrigins = false;
        private boolean allowAllHeaders = false;

        public static CorsConfiguration createDefault() {
            CorsConfiguration config = new CorsConfiguration();
            config.setAllowedMethods(Arrays.asList("GET", "POST", "PUT", "DELETE", "OPTIONS", "PATCH"));
            config.setAllowedHeaders(Arrays.asList("Content-Type", "Authorization", "X-Requested-With", "Accept"));
            return config;
        }

        public CorsConfiguration addAllowedOrigin(String origin) {
            if ("*".equals(origin)) {
                this.allowAllOrigins = true;
            } else {
                this.allowedOrigins.add(origin.trim().toLowerCase(Locale.ROOT));
            }
            return this;
        }

        public CorsConfiguration setAllowedMethods(List<String> methods) {
            this.allowedMethods.clear();
            for (String method : methods) {
                this.allowedMethods.add(method.trim().toUpperCase(Locale.ROOT));
            }
            return this;
        }

        public CorsConfiguration setAllowedHeaders(List<String> headers) {
            this.allowedHeaders.clear();
            for (String header : headers) {
                if ("*".equals(header.trim())) {
                    this.allowAllHeaders = true;
                } else {
                    this.allowedHeaders.add(header.trim().toLowerCase(Locale.ROOT));
                }
            }
            return this;
        }

        public CorsConfiguration setExposedHeaders(List<String> headers) {
            this.exposedHeaders.clear();
            for (String header : headers) {
                this.exposedHeaders.add(header.trim());
            }
            return this;
        }

        public CorsConfiguration setAllowCredentials(boolean allowCredentials) {
            this.allowCredentials = allowCredentials;
            return this;
        }

        public CorsConfiguration setMaxAge(long maxAgeSeconds) {
            this.maxAgeSeconds = maxAgeSeconds;
            return this;
        }

        public boolean isOriginAllowed(String origin) {
            if (origin == null || origin.isEmpty()) {
                return false;
            }
            if (allowAllOrigins && !allowCredentials) {
                return true;
            }
            return allowedOrigins.contains(origin.trim().toLowerCase(Locale.ROOT)) || allowAllOrigins;
        }

        public boolean isMethodAllowed(String method) {
            if (method == null) {
                return false;
            }
            return allowedMethods.contains(method.trim().toUpperCase(Locale.ROOT));
        }

        public boolean areHeadersAllowed(List<String> requestHeaders) {
            if (allowAllHeaders || requestHeaders == null || requestHeaders.isEmpty()) {
                return true;
            }
            for (String headerList : requestHeaders) {
                String[] split = headerList.split(",");
                for (String h : split) {
                    String trimmed = h.trim().toLowerCase(Locale.ROOT);
                    if (!trimmed.isEmpty() && !allowedHeaders.contains(trimmed)) {
                        return false;
                    }
                }
            }
            return true;
        }

        public Set<String> getAllowedOrigins() {
            return Collections.unmodifiableSet(allowedOrigins);
        }

        public Set<String> getAllowedMethods() {
            return Collections.unmodifiableSet(allowedMethods);
        }

        public Set<String> getAllowedHeaders() {
            return Collections.unmodifiableSet(allowedHeaders);
        }

        public Set<String> getExposedHeaders() {
            return Collections.unmodifiableSet(exposedHeaders);
        }

        public boolean isAllowCredentials() {
            return allowCredentials;
        }

        public long getMaxAgeSeconds() {
            return maxAgeSeconds;
        }

        public boolean isAllowAllOrigins() {
            return allowAllOrigins;
        }

        public boolean isAllowAllHeaders() {
            return allowAllHeaders;
        }
    }

    public static class CorsFilterHandler implements HttpHandler {
        private final HttpHandler nextHandler;
        private final CorsConfiguration config;

        public CorsFilterHandler(CorsConfiguration config, HttpHandler nextHandler) {
            this.config = config != null ? config : CorsConfiguration.createDefault();
            this.nextHandler = nextHandler;
        }

        @Override
        public void handle(HttpExchange exchange) throws IOException {
            Headers reqHeaders = exchange.getRequestHeaders();
            String origin = reqHeaders.getFirst("Origin");
            String requestMethod = exchange.getRequestMethod();

            if (origin == null || origin.trim().isEmpty()) {
                nextHandler.handle(exchange);
                return;
            }

            if ("OPTIONS".equalsIgnoreCase(requestMethod) && reqHeaders.containsKey("Access-Control-Request-Method")) {
                handlePreflightRequest(exchange, origin);
            } else {
                handleSimpleOrActualRequest(exchange, origin);
            }
        }

        private void handlePreflightRequest(HttpExchange exchange, String origin) throws IOException {
            Headers reqHeaders = exchange.getRequestHeaders();
            Headers respHeaders = exchange.getResponseHeaders();

            if (!config.isOriginAllowed(origin)) {
                exchange.sendResponseHeaders(403, -1);
                exchange.close();
                return;
            }

            String requestedMethod = reqHeaders.getFirst("Access-Control-Request-Method");
            if (requestedMethod == null || !config.isMethodAllowed(requestedMethod)) {
                exchange.sendResponseHeaders(403, -1);
                exchange.close();
                return;
            }

            List<String> requestedHeaders = reqHeaders.get("Access-Control-Request-Headers");
            if (requestedHeaders != null && !config.areHeadersAllowed(requestedHeaders)) {
                exchange.sendResponseHeaders(403, -1);
                exchange.close();
                return;
            }

            applyCorsHeaders(respHeaders, origin);

            respHeaders.set("Access-Control-Allow-Methods", String.join(", ", config.getAllowedMethods()));

            if (config.isAllowAllHeaders() && requestedHeaders != null) {
                respHeaders.set("Access-Control-Allow-Headers", String.join(", ", requestedHeaders));
            } else if (!config.getAllowedHeaders().isEmpty()) {
                respHeaders.set("Access-Control-Allow-Headers", String.join(", ", config.getAllowedHeaders()));
            }

            if (config.getMaxAgeSeconds() > 0) {
                respHeaders.set("Access-Control-Max-Age", String.valueOf(config.getMaxAgeSeconds()));
            }

            exchange.sendResponseHeaders(204, -1);
            exchange.close();
        }

        private void handleSimpleOrActualRequest(HttpExchange exchange, String origin) throws IOException {
            Headers respHeaders = exchange.getResponseHeaders();

            if (config.isOriginAllowed(origin)) {
                applyCorsHeaders(respHeaders, origin);

                if (!config.getExposedHeaders().isEmpty()) {
                    respHeaders.set("Access-Control-Expose-Headers", String.join(", ", config.getExposedHeaders()));
                }
            }

            nextHandler.handle(exchange);
        }

        private void applyCorsHeaders(Headers respHeaders, String origin) {
            if (config.isAllowAllOrigins() && !config.isAllowCredentials()) {
                respHeaders.set("Access-Control-Allow-Origin", "*");
            } else {
                respHeaders.set("Access-Control-Allow-Origin", origin);
                respHeaders.add("Vary", "Origin");
            }

            if (config.isAllowCredentials()) {
                respHeaders.set("Access-Control-Allow-Credentials", "true");
            }
        }
    }

    public static class SampleApiHandler implements HttpHandler {
        @Override
        public void handle(HttpExchange exchange) throws IOException {
            String method = exchange.getRequestMethod();
            String responseBody;
            int statusCode = 200;

            if ("GET".equalsIgnoreCase(method)) {
                responseBody = "{\"status\":\"success\",\"message\":\"CORS-enabled data retrieved.\"}";
            } else if ("POST".equalsIgnoreCase(method)) {
                InputStream is = exchange.getRequestBody();
                String body = new String(is.readAllBytes(), StandardCharsets.UTF_8);
                responseBody = "{\"status\":\"created\",\"received\":" + (body.isEmpty() ? "{}" : body) + "}";
                statusCode = 201;
            } else {
                responseBody = "{\"status\":\"unsupported_method\"}";
                statusCode = 405;
            }

            byte[] bytes = responseBody.getBytes(StandardCharsets.UTF_8);
            exchange.getResponseHeaders().set("Content-Type", "application/json; charset=UTF-8");
            exchange.getResponseHeaders().set("X-Custom-Header", "DemoCorsValue");
            exchange.sendResponseHeaders(statusCode, bytes.length);

            try (OutputStream os = exchange.getResponseBody()) {
                os.write(bytes);
            }
        }
    }

    public static void main(String[] args) throws Exception {
        int port = 8080;
        HttpServer server = HttpServer.create(new InetSocketAddress(port), 0);

        CorsConfiguration corsConfig = new CorsConfiguration()
                .addAllowedOrigin("https://app.example.com")
                .addAllowedOrigin("http://localhost:3000")
                .setAllowedMethods(Arrays.asList("GET", "POST", "PUT", "DELETE", "OPTIONS"))
                .setAllowedHeaders(Arrays.asList("Content-Type", "Authorization", "X-Requested-With"))
                .setExposedHeaders(Arrays.asList("X-Custom-Header"))
                .setAllowCredentials(true)
                .setMaxAge(3600);

        HttpHandler apiHandler = new SampleApiHandler();
        HttpHandler corsProtectedHandler = new CorsFilterHandler(corsConfig, apiHandler);

        server.createContext("/api/data", corsProtectedHandler);
        server.setExecutor(Executors.newFixedThreadPool(4));
        server.start();

        System.out.println("CORS API Server started on http://localhost:" + port + "/api/data");

        runVerificationTests(port);

        server.stop(0);
        System.out.println("Server stopped successfully.");
    }

    private static void runVerificationTests(int port) throws Exception {
        System.out.println("\n--- Test 1: Preflight Request (OPTIONS) with Allowed Origin ---");
        executeRequest("OPTIONS", port, "https://app.example.com", "POST", "Authorization, Content-Type", null);

        System.out.println("\n--- Test 2: Actual GET Request with Allowed Origin ---");
        executeRequest("GET", port, "https://