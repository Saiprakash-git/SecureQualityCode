import com.sun.net.httpserver.HttpExchange;
import com.sun.net.httpserver.HttpHandler;
import com.sun.net.httpserver.HttpServer;
import java.io.IOException;
import java.io.OutputStream;
import java.net.InetSocketAddress;
import java.nio.charset.StandardCharsets;
import java.util.HashMap;
import java.util.Map;

public class CorsApiServer {

    public static void main(String[] args) throws IOException {
        int port = args.length > 0 ? Integer.parseInt(args[0]) : 8080;
        HttpServer server = startServer(port);
        System.out.println("CORS API Server running on port " + port);
    }

    public static HttpServer startServer(int port) throws IOException {
        HttpServer server = HttpServer.create(new InetSocketAddress(port), 0);
        server.createContext("/api", new CorsHandler());
        server.setExecutor(null);
        server.start();
        return server;
    }

    public static Map<String, String> generateCorsHeaders(String origin) {
        Map<String, String> headers = new HashMap<>();
        headers.put("Access-Control-Allow-Origin", origin != null ? origin : "*");
        headers.put("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        headers.put("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With, Accept");
        headers.put("Access-Control-Max-Age", "86400");
        headers.put("Access-Control-Allow-Credentials", "true");
        return headers;
    }

    public static int handlePreflight(String method, String origin) {
        if ("OPTIONS".equalsIgnoreCase(method)) {
            return 204;
        }
        return 405;
    }

    public static String processApiRequest(String method, String path, String requestBody) {
        switch (method.toUpperCase()) {
            case "GET":
                return "{\"status\":\"success\",\"message\":\"GET request processed\",\"path\":\"" + path + "\"}";
            case "POST":
                return "{\"status\":\"success\",\"message\":\"POST request processed\",\"received\":\"" + (requestBody != null ? requestBody : "empty") + "\"}";
            case "PUT":
                return "{\"status\":\"success\",\"message\":\"PUT request processed\"}";
            case "DELETE":
                return "{\"status\":\"success\",\"message\":\"DELETE request processed\"}";
            default:
                return "{\"error\":\"Method not allowed\"}";
        }
    }

    static class CorsHandler implements HttpHandler {
        @Override
        public void handle(HttpExchange exchange) throws IOException {
            String origin = exchange.getRequestHeaders().getFirst("Origin");
            String method = exchange.getRequestMethod();
            String path = exchange.getRequestURI().getPath();

            Map<String, String> corsHeaders = generateCorsHeaders(origin);
            for (Map.Entry<String, String> entry : corsHeaders.entrySet()) {
                exchange.getResponseHeaders().set(entry.getKey(), entry.getValue());
            }

            if ("OPTIONS".equalsIgnoreCase(method)) {
                int status = handlePreflight(method, origin);