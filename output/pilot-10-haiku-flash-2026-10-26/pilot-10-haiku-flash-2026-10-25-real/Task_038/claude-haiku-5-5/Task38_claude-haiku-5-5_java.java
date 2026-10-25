import com.sun.net.httpserver.HttpExchange;
import com.sun.net.httpserver.HttpServer;

import java.io.IOException;
import java.io.OutputStream;
import java.net.InetSocketAddress;
import java.net.URLDecoder;
import java.nio.charset.StandardCharsets;
import java.util.LinkedHashMap;
import java.util.Map;

public class Main {

    public static void main(String[] args) throws IOException {
        int port = args.length > 0 ? Integer.parseInt(args[0]) : 8080;
        HttpServer server = HttpServer.create(new InetSocketAddress(port), 0);
        server.createContext("/", Main::handle);
        server.setExecutor(null);
        server.start();
        System.out.println("Server running at http://localhost:" + port + "/");
    }

    static void handle(HttpExchange exchange) throws IOException {
        try {
            String method = exchange.getRequestMethod();
            String path = exchange.getRequestURI().getPath();

            if (!"/".equals(path)) {
                send(exchange, 404, page("Not Found", "<p>The page you requested does not exist.</p><p><a href=\"/\">Go to form</a></p>"));
                return;
            }

            if ("GET".equalsIgnoreCase(method)) {
                send(exchange, 200, formPage());
            } else if ("POST".equalsIgnoreCase(method)) {
                String body = readBody(exchange);
                Map<String, String> data = parseForm(body);
                send(exchange, 200, resultPage(data));
            } else {
                send(exchange, 405, page("Method Not Allowed", "<p>Only GET and POST are supported.</p>"));
            }
        } catch (Exception e) {
            send(exchange, 500, page("Error", "<p>An unexpected error occurred: " + escape(e.getMessage()) + "</p>"));
        } finally {
            exchange.close();
        }
    }

    static String readBody(HttpExchange exchange) throws IOException {
        return new String(exchange.getRequestBody().readAllBytes(), StandardCharsets.UTF_8);
    }

    static Map<String, String> parseForm(String body) {
        Map<String, String> result = new LinkedHashMap<>();
        if (body == null || body.isEmpty()) {
            return result;
        }
        for (String pair : body.split("&")) {
            if (pair.isEmpty()) continue;
            int idx = pair.indexOf('=');
            String rawKey = idx >= 0 ? pair.substring(0, idx) : pair;
            String rawValue = idx >= 0 ? pair.substring(idx + 1) : "";
            String key = URLDecoder.decode(rawKey, StandardCharsets.UTF_8);
            String value = URLDecoder.decode(rawValue, StandardCharsets.UTF_8);
            result.put(key, value);
        }
        return result;
    }

    static String escape(String input) {
        if (input == null) return "";
        StringBuilder sb = new StringBuilder(input.length());
        for (char c : input.toCharArray()) {
            switch (c) {
                case '&': sb.append("&amp;"); break;
                case '<': sb.append("&lt;"); break;
                case '>': sb.append("&gt;"); break;
                case '"': sb.append("&quot;"); break;
                case '\'': sb.append("&#x27;"); break;
                default: sb.append(c);
            }
        }
        return sb.toString();
    }

    static String formPage() {
        String content = """
                <h1>Feedback Form</h1>
                <form method="post" action="/">
                  <div class="field">
                    <label for="name">Full name</label>
                    <input type="text" id="name" name="name" required>
                  </div>
                  <div class="field">
                    <label for="email">Email</label>
                    <input type="email" id="email" name="email" required>
                  </div>
                  <div class="field">
                    <label for="age">Age</label>
                    <input type="number" id="age" name="age" min="1" max="150">
                  </div>
                  <div class="field">
                    <label for="topic">Topic</label>
                    <select id="topic" name="topic">
                      <option value="General">General</option>
                      <option value="Support">Support</option>
                      <option value="Sales">Sales</option>
                      <option value="Other">Other</option>
                    </select>
                  </div>
                  <div class="field">
                    <label for="message">Message</label>
                    <textarea id="message" name="message" rows="5" required></textarea>
                  </div>
                  <button type="submit">Submit</button>
                </form>
                """;
        return page("Form", content);
    }

    static String resultPage(Map<String, String> data) {
        StringBuilder rows = new StringBuilder();
        for (Map.Entry<String, String> entry : data.entrySet()) {
            rows.append("<tr><th>")
                .append(escape(entry.getKey()))
                .append("</th><td>")
                .append(escape(entry.getValue()).replace("\n", "<br>"))
                .append("</td></tr>\n");
        }
        if (rows.length() == 0) {
            rows.append("<tr><td colspan=\"2\">No data was submitted.</td></tr>");
        }

        String content = """
                <h1>Submitted Data</h1>
                <table>
                %s
                </table>
                <p><a href="/">Submit another response</a></p>
                """.formatted(rows);
        return page("Result", content);
    }

    static String page(String title, String content) {
        return """
                <!DOCTYPE html>
                <html lang="en">
                <head>
                  <meta charset="UTF-8">
                  <meta name="viewport" content="width=device-width, initial-scale=1.0">
                  <title>%s</title>
                  <style>
                    body { font-family: Arial, Helvetica, sans-serif; background: #f4f6f8; margin: 0; padding: 20px; color: #222; }
                    .container { max-width: 600px; margin: 30px auto; background: #fff; padding: 25px 30px; border-radius: 8px; box-shadow: 0 2px 8px rgba(0,0,0,0.1); }
                    h1 { margin-top: 0; font-size: 1.6em; }
                    .field { margin-bottom: 15px; }
                    label { display: block; font-weight: bold; margin-bottom: 5px; }
                    input, select, textarea { width: 100%%; padding: 8px; box-sizing: border-box; border: 1px solid #ccc; border-radius: 4px; font-size: 1em; }
                    button { background: #2d6cdf; color: #fff; border: none; padding: 10px 20px; border-radius: 4px; font-size: 1em; cursor: pointer; }
                    button:hover { background: #1f54b8; }
                    table { width: 100%%; border-collapse: collapse; }
                    th, td { text-align: left; padding: 10px; border-bottom: 1px solid #ddd; vertical-align: top; }
                    th { width: 30%%; background: #fafafa; }
                    a { color: #2d6cdf; }
                  </style>
                </head>
                <body>
                  <div class="container">
                %s
                  </div>
                </body>
                </html>
                """.formatted(escape(title), content);
    }

    static void send(HttpExchange exchange, int status, String html) throws IOException {
        byte[] bytes = html.getBytes(StandardCharsets.UTF_8);
        exchange.getResponseHeaders().set("Content-Type", "text/html; charset=UTF-8");
        exchange.sendResponseHeaders(status, bytes.length);
        try (OutputStream os = exchange.getResponseBody()) {
            os.write(bytes);
        }
    }
}