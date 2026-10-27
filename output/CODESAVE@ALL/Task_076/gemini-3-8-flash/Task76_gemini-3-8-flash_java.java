import java.io.IOException;
import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.time.Duration;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class PageTitleExtractor {

    private static final Pattern TITLE_PATTERN = Pattern.compile(
            "<title[^>]*>(.*?)</title>",
            Pattern.CASE_INSENSITIVE | Pattern.DOTALL
    );

    public static String fetchPageContent(String urlString) throws IOException, InterruptedException {
        HttpClient client = HttpClient.newBuilder()
                .connectTimeout(Duration.ofSeconds(10))
                .followRedirects(HttpClient.Redirect.NORMAL)
                .build();

        HttpRequest request = HttpRequest.newBuilder()
                .uri(URI.create(urlString))
                .timeout(Duration.ofSeconds(15))
                .header("User-Agent", "Mozilla/5.0")
                .GET()
                .build();

        HttpResponse<String> response = client.send(request, HttpResponse.BodyHandlers.ofString());
        return response.body();
    }

    public static String extractTitleFromHtml(String html) {
        if (html == null) {
            return null;
        }
        Matcher matcher = TITLE_PATTERN.matcher(html);
        if (matcher.find()) {
            return matcher.group(1).trim().replaceAll("\\s+", " ");
        }
        return null;
    }

    public static String getPageTitle(String urlString) throws IOException, InterruptedException {
        String html = fetchPageContent(urlString);
        return extractTitleFromHtml(html);
    }

    public static void main(String[] args) {
        if (args.length == 0) {
            System.err.println("Usage: java PageTitleExtractor <HTTPS_URL>");
            System.exit(1);
        }

        String targetUrl = args[0];
        try {
            String title = getPageTitle(targetUrl);
            if (title != null) {
                System.out.println(title);
            } else {
                System.out.println("No title found.");
            }
        } catch (Exception e) {
            System.err.println("Error fetching page title: " + e.getMessage());
            System.exit(1);
        }
    }
}