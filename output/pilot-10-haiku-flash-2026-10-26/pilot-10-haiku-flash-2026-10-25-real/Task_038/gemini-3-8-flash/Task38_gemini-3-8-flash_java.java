import com.sun.net.httpserver.HttpExchange;
import com.sun.net.httpserver.HttpHandler;
import com.sun.net.httpserver.HttpServer;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.UnsupportedEncodingException;
import java.net.InetSocketAddress;
import java.net.URLDecoder;
import java.nio.charset.StandardCharsets;
import java.util.HashMap;
import java.util.Map;

public class WebApp {

    public static void main(String[] args) throws IOException {
        int port = 8080;
        HttpServer server = createServer(port);
        server.start();
        System.out.println("Server running at http://localhost:" + port + "/");
    }

    public static HttpServer createServer(int port) throws IOException {
        HttpServer server = HttpServer.create(new InetSocketAddress(port), 0);
        server.createContext("/", new FormHandler());
        server.setExecutor(null);
        return server;
    }

    public static class FormHandler implements HttpHandler {
        @Override
        public void handle(HttpExchange exchange) throws IOException {
            String method = exchange.getRequestMethod();
            String submittedText = null;

            if ("POST".equalsIgnoreCase(method)) {
                String requestBody = readRequestBody(exchange.getRequestBody());
                Map<String, String> formData = parseFormData(requestBody);
                submittedText = formData.get("userInput");
            }

            String htmlResponse = generateHtmlPage(submittedText);
            sendResponse(exchange, 200, htmlResponse);
        }

        public String readRequestBody(InputStream inputStream) throws IOException {
            StringBuilder sb = new StringBuilder();
            try (BufferedReader reader = new BufferedReader(new InputStreamReader(inputStream, StandardCharsets.UTF_8))) {
                String line;
                while ((line = reader.readLine()) != null) {
                    sb.append(line);
                }
            }
            return sb.toString();
        }

        public Map<String, String> parseFormData(String formData) throws UnsupportedEncodingException {
            Map<String, String> map = new HashMap<>();
            if (formData == null || formData.isEmpty()) {
                return map;
            }
            String[] pairs = formData.split("&");
            for (String pair : pairs) {
                String[] keyValue = pair.split("=", 2);
                String key = URLDecoder.decode(keyValue[0], StandardCharsets.UTF_8.name());
                String value = keyValue.length > 1 ? URLDecoder.decode(keyValue[1], StandardCharsets.UTF_8.name()) : "";
                map.put(key, value);
            }
            return map;
        }

        public String escapeHtml(String text) {
            if (text == null) {
                return "";
            }
            return text.replace("&", "&amp;")
                       .replace("<", "&lt;")
                       .replace(">", "&gt;")
                       .replace("\"", "&quot;")
                       .replace("'", "&#39;");
        }

        public String generateHtmlPage(String displayedText) {
            StringBuilder html = new StringBuilder();
            html.append("<!DOCTYPE html>\n")
                .append("<html lang=\"en\">\n")
                .append("<head>\n")
                .append("    <meta charset=\"UTF-8\">\n")
                .append("    <title>Echo Form Web Application</title>\n")
                .append("    <style>\n")
                .append("        body { font-family: Arial, sans-serif; margin: 40px; background-color: #f7f9fa; color: #333; }\n")
                .append("        .container { max-width: 500px; margin: auto; background: white; padding: 24px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }\n")
                .append("        input[type=\"text\"] { width: 100%; padding: 10px; margin: 8px 0; box-sizing: border-box; border: 1px solid #ccc; border-radius: 4px; }\n")
                .append("        input[type=\"submit\"] { background-color: #007bff; color: white; padding: 10px 16px; border: none; border-radius: 4px; cursor: pointer; }\n")
                .append("        input[type=\"submit\"]:hover { background-color: #0056b3; }\n")
                .append("        .result { margin-top: 20px; padding: 12px; background-color: #e9ecef; border-radius: 4px; border-left: 4px solid #007bff; }\n")
                .append("    </style>\n")
                .append("</head>\n")
                .append("<body>\n")
                .append("    <div class=\"container\">\n")
                .append("        <h2>Submit Input</h2>\n")
                .append("        <form method=\"POST\" action=\"/\">\n")
                .append("            <label for=\"userInput\">Enter your message:</label>\n")
                .append("            <input type=\"text\" id=\"userInput\" name=\"userInput\" required />\n")
                .append("            <input type=\"submit\" value=\"Submit\" />\n")
                .append("        </form>\n");

            if (displayedText != null) {
                html.append("        <div class=\"result\">\n")
                    .append("            <h3>You Submitted:</h3>\n")
                    .append("            <p>").append(escapeHtml(displayedText)).append("</p>\n")
                    .append("        </div>\n");
            }

            html.append("    </div>\n")
                .append("</body>\n")
                .append("</html>");

            return html.toString();
        }

        public void sendResponse(HttpExchange exchange, int statusCode, String response) throws IOException {
            byte[] bytes = response.getBytes(StandardCharsets.UTF_8);
            exchange.getResponseHeaders().set("Content-Type", "text/html; charset=UTF-8");
            exchange.sendResponseHeaders(statusCode, bytes.length);
            try (OutputStream os = exchange.getResponseBody()) {
                os.write(bytes);
            }
        }
    }
}