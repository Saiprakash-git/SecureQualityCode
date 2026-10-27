import com.sun.net.httpserver.Headers;
import com.sun.net.httpserver.HttpExchange;
import com.sun.net.httpserver.HttpHandler;
import com.sun.net.httpserver.HttpServer;

import java.io.IOException;
import java.io.OutputStream;
import java.net.InetSocketAddress;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.CopyOnWriteArrayList;

public class CorsServer {

    static final class CorsPolicy {
        private final Set<String> allowedOrigins;
        private final Set<String> allowedMethods;
        private final Set<String> allowedHeaders;
        private final Set<String> exposedHeaders;
        private final boolean allowCredentials;
        private final long maxAgeSeconds;

        CorsPolicy(Set<String> allowedOrigins, Set<String> allowedMethods, Set<String> allowedHeaders,
                   Set<String> exposedHeaders, boolean allowCredentials, long maxAgeSeconds) {
            this.allowedOrigins = normalize(allowedOrigins, false);
            this.allowedMethods = normalize(allowedMethods, true);
            this.allowedHeaders = normalize(allowedHeaders, false);
            this.exposedHeaders = new LinkedHashSet<>(exposedHeaders == null ? Collections.emptySet() : exposedHeaders);
            this.allowCredentials = allowCredentials;
            this.maxAgeSeconds = Math.max(0, maxAgeSeconds);
        }

        private static Set<String> normalize(Set<String> input, boolean upper) {
            Set<String> out = new LinkedHashSet<>();
            if (input == null) return out;
            for (String s : input) {
                if (s == null || s.isBlank()) continue;
                String t = s.trim();
                out.add(upper ? t.toUpperCase() : t.toLowerCase());
            }
            return out;
        }

        boolean isWildcardOrigin() {
            return allowedOrigins.contains("*");
        }

        boolean isOriginAllowed(String origin) {
            return isWildcardOrigin() || allowedOrigins.contains(origin.trim().toLowerCase());
        }

        boolean isMethodAllowed(String method) {
            return method != null && allowedMethods.contains(method.trim().toUpperCase());
        }

        boolean areHeadersAllowed(String requestedHeaders) {
            if (requestedHeaders == null || requestedHeaders.isBlank()) return true;
            if (allowedHeaders.contains("*")) return true;
            for (String h : requestedHeaders.split(",")) {
                String name = h.trim().toLowerCase();
                if (name.isEmpty()) continue;
                if (!allowedHeaders.contains(name)) return false;
            }
            return true;
        }

        Result evaluate(String origin, String method, String requestMethod, String requestHeaders) {
            if (origin == null || origin.isBlank()) {
                return new Result(false, true, Collections.emptyMap());
            }

            if (!isOriginAllowed(origin)) {
                Map<String, String> h = new LinkedHashMap<>();
                h.put("Vary", "Origin");
                return new Result(true, false, h);
            }

            boolean preflight = "OPTIONS".equalsIgnoreCase(method) && requestMethod != null;
            Map<String, String> h = new LinkedHashMap<>();

            if (preflight) {
                h.put("Vary", "Origin, Access-Control-Request-Method, Access-Control-Request-Headers");
            } else {
                h.put("Vary", "Origin");
            }

            String allowOrigin = (isWildcardOrigin() && !allowCredentials) ? "*" : origin;
            h.put("Access-Control-Allow-Origin", allowOrigin);

            if (allowCredentials) {
                h.put("Access-Control-Allow-Credentials", "true");
            }

            if (preflight) {
                boolean ok = isMethodAllowed(requestMethod) && areHeadersAllowed(requestHeaders);
                if (ok) {
                    h.put("Access-Control-Allow-Methods", String.join(", ", allowedMethods));
                    if (!allowedHeaders.isEmpty()) {
                        h.put("Access-Control-Allow-Headers", String.join(", ", allowedHe