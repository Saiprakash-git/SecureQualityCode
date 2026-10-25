import com.sun.net.httpserver.HttpExchange;
import com.sun.net.httpserver.HttpServer;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.InetSocketAddress;
import java.net.URLDecoder;
import java.nio.charset.StandardCharsets;
import java.util.HashMap;
import java.util.Map;

public class Main {
    private static final int MAX_REQUEST_SIZE = 1_000_000;

    public static void main(String[] args) throws IOException {
        int port = args.length > 0 ? Integer.parseInt(args[0]) : 8080;
        HttpServer server = HttpServer.create(new InetSocketAddress(port), 0);

        server.createContext("/", exchange -> {
            if (!"/".equals(exchange.getRequestURI().getPath())) {
                send(exchange, 404, page("", ""));
                return;
            }

            if ("GET".equalsIgnoreCase(exchange.getRequestMethod())) {
                send(exchange, 200, page("", ""));
            } else {
                send(exchange, 405, "Method not allowed.");
            }
        });

        server.createContext("/submit", exchange -> {
            if (!"POST".equalsIgnoreCase(exchange.getRequestMethod())) {
                send(exchange, 405, "Method not allowed.");
                return;
            }

            try {
                String body = readBody(exchange.getRequestBody());
                Map<String, String> form = parseForm(body);
                send(exchange, 200, page(form.getOrDefault("name", ""), form.getOrDefault("message", "")));
            } catch (IllegalArgumentException e) {
                send(exchange, 400, "Invalid form submission.");
            } catch (IOException e) {
                send(exchange, 500, "Unable to read form submission.");
            }
        });

        server.start();
        System.out.println("Open http://localhost:" + port);
    }

    private static String page(String name, String message) {
        String escapedName = escapeHtml(name);
        String escapedMessage = escapeHtml(message);

        return "<!doctype html>"
                + "<html lang=\"en\"><head><meta charset=\"utf-8\">"
                + "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
                + "<title>Echo Form</title>"
                + "<style>body{font:16px sans-serif;max-width:640px;margin:3rem auto;padding:0 1rem}"
                + "label{display:block;margin:1rem 0 .35rem}input,textarea{box-sizing:border-box;width:100%;padding:.6rem}"
                + "button{margin-top:1rem;padding:.6rem 1rem}.result{margin-top:2rem;padding:1rem;background:#f2f2f2}"
                + "pre{white-space:pre-wrap;overflow-wrap:anywhere}</style></head><body>"
                + "<h1>Echo Form</h1>"
                + "<form action=\"/submit\" method=\"post\">"
                + "<label for=\"name\">Name</label><input id=\"name\" name=\"name\" value=\""
                + escapedName + "\">"
                + "<label for=\"message\">Message</label><textarea id=\"message\" name=\"message\" rows=\"5\">"
                + escapedMessage + "</textarea>"
                + "<button type=\"submit\">Submit</button></form>"
                + (name.isEmpty() && message.isEmpty() ? "" :
                "<section class=\"result\"><h2>Your submission</h2><p><strong>Name:</strong> "
                        + escapedName + "</p><p><strong>Message:</strong></p><pre>"
                        + escapedMessage + "</pre></section>")
                + "</body></html>";
    }

    private static Map<String, String> parseForm(String body) {
        Map<String, String> values = new HashMap<>();
        if (body.isEmpty()) {
            return values;
        }

        for (String pair : body.split("&")) {
            String[] parts = pair.split("=", 2);
            String key = decode(parts[0]);
            String value = parts.length > 1 ? decode(parts[1]) : "";
            values.put(key, value);
        }
        return values;
    }

    private static String decode(String value) {
        return URLDecoder.decode(value, StandardCharsets.UTF_8);
    }

    private static String escapeHtml(String value) {
        return value.replace("&", "&amp;")
                .replace("<", "&lt;")
                .replace(">", "&gt;")
                .replace("\"", "&quot;")
                .replace("'", "&#39;");
    }

    private static String readBody(InputStream input) throws IOException {
        ByteArrayOutputStream output = new ByteArrayOutputStream();
        byte[] buffer = new byte[4096];
        int total = 0;
        int count;

        while ((count = input.read(buffer)) != -1) {
            total += count;
            if (total > MAX_REQUEST_SIZE) {
                throw new IllegalArgumentException("Request too large");
            }
            output.write(buffer, 0, count);
        }

        return output.toString(StandardCharsets.UTF_8);
    }

    private static void send(HttpExchange exchange, int status, String content) throws IOException {
        byte[] response = content.getBytes(StandardCharsets.UTF_8);
        exchange.getResponseHeaders().set("Content-Type", "text/html; charset=UTF-8");
        exchange.getResponseHeaders().set("X-Content-Type-Options", "nosniff");
        exchange.sendResponseHeaders(status, response.length);
        exchange.getResponseBody().write(response);
        exchange.close();
    }
}