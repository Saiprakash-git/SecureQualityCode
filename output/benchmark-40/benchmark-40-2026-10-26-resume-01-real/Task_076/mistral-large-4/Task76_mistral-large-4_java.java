import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.time.Duration;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class WebpageTitleExtractor {

    public static String extractTitle(String urlString) throws Exception {
        HttpClient client = HttpClient.newBuilder()
                .followRedirects(HttpClient.Redirect.NORMAL)
                .connectTimeout(Duration.ofSeconds(10))
                .build();

        HttpRequest request = HttpRequest.newBuilder()
                .uri(URI.create(urlString))
                .timeout(Duration.ofSeconds(30))
                .header("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36")
                .GET()
                .build();

        HttpResponse<String> response = client.send(request, HttpResponse.BodyHandlers.ofString());

        if (response.statusCode() >= 400) {
            throw new RuntimeException("HTTP error code: " + response.statusCode());
        }

        String html = response.body();
        Pattern pattern = Pattern.compile("<title[^>]*>(.*?)</title>", Pattern.CASE_INSENSITIVE | Pattern.DOTALL);
        Matcher matcher = pattern.matcher(html);

        if (matcher.find()) {
            return matcher.group(1).trim();
        }

        return "";
    }

    public static void main(String[] args) {
        if (args.length < 1) {
            System.err.println("Usage: java WebpageTitleExtractor <https-url>");
            System.exit(1);
        }

        try {
            String title = extractTitle(args[0]);
            System.out.println(title);
        } catch (Exception e) {
            System.err.println("Error: " + e.getMessage());
            System.exit(1);
        }
    }
}