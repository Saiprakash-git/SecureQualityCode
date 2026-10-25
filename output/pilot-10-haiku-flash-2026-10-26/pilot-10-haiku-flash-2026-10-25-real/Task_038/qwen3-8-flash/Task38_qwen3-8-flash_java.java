import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.UnsupportedEncodingException;
import java.net.InetSocketAddress;
import java.net.URLDecoder;
import java.nio.charset.StandardCharsets;
import java.util.LinkedHashMap;
import java.util.Map;

import com.sun.net.httpserver.HttpExchange;
import com.sun.net.httpserver.HttpHandler;
import com.sun.net.httpserver.HttpServer;

public class Main {
    public static void main(String[] args) throws Exception {
        int port = 8080;
        if (args.length > 0) {
            port = Integer.parseInt(args[0]);
        }

        HttpServer server = HttpServer.create(new InetSocketAddress(port), 0);
        server.createContext("/", new FormHandler());
        server.setExecutor(null);
        server.start();

        System.out.println("Server started on http://localhost:" + port);
    }
}

class FormHandler implements HttpHandler {
    @Override
    public void handle(HttpExchange exchange) throws IOException {
        String method = exchange.getRequestMethod();
        String path = exchange.getRequestURI().getPath();
        String query = exchange.getRequestURI().getRawQuery();

        Map<String, String> values = new LinkedHashMap<>();

        if ("/submit".equals(path) && "POST".equalsIgnoreCase(method)) {
            values.putAll(parseParams(readBody(exchange)));
        } else if (query != null && !query.isEmpty()) {
            values.putAll(parseParams(query));
        }

        String html;
        if (!values.isEmpty()) {
            html = renderResult(values);
        } else {
            html = renderForm();
        }

        byte[] response = html.getBytes(StandardCharsets.UTF_8);
        exchange.getResponseHeaders().set("Content-Type", "text/html; charset=UTF-8");
        exchange.sendResponseHeaders(200, response.length);

        try (OutputStream os = exchange.getResponseBody()) {
            os.write(response);
        }
    }

    private static String readBody(HttpExchange exchange) throws IOException {
        ByteArrayOutputStream out = new ByteArrayOutputStream();

        try (InputStream in = exchange.getRequestBody()) {
            byte[] buffer = new byte[1024];
            int read;
            while ((read = in.read(buffer)) != -1) {
                out.write(buffer, 0, read);
            }
        }

        return new String(out.toByteArray(), StandardCharsets.UTF_8);
    }

    private static Map<String, String> parseParams(String data) {
        Map<String, String> map = new LinkedHashMap<>();

        if (data == null || data.isEmpty()) {
            return map;
        }

        String[] pairs = data.split("&");

        for (String pair : pairs) {
            if (pair.isEmpty()) {
                continue;
            }

            int equalsIndex = pair.indexOf('=');
            String rawKey = equalsIndex >= 0 ? pair.substring(0, equalsIndex) : pair;
            String rawValue = equalsIndex >= 0 ? pair.substring(equalsIndex + 1) : "";

            try {
                String key = URLDecoder.decode(rawKey, StandardCharsets.UTF_8.name());
                String value = URLDecoder.decode(rawValue, StandardCharsets.UTF_8.name());
                map.put(key, value);
            } catch (UnsupportedEncodingException e) {
                map.put(rawKey, rawValue);
            }
        }

        return map;
    }

    private static String renderForm() {
        return "<!DOCTYPE html>\n" +
                "<html lang=\"en\">\n" +
                "<head>\n" +
                "  <meta charset