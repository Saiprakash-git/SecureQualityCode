import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.time.Duration;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class PageTitleFetcher {

    private static final Pattern TITLE_PATTERN =
            Pattern.compile("<title[^>]*>(.*?)</title>", Pattern.CASE_INSENSITIVE | Pattern.DOTALL);

    public static String fetchPageContent(String url) throws Exception {
        if (url == null || !url.trim().toLowerCase().startsWith("https://")) {
            throw new IllegalArgumentException("URL must be an HTTPS URL: " + url);
        }

        HttpClient client = HttpClient.newBuilder()
                .followRedirects(HttpClient.Redirect.NORMAL)
                .connectTimeout(Duration.ofSeconds(15))
                .build();

        HttpRequest request = HttpRequest.newBuilder()
                .uri(URI.create(url.trim()))
                .timeout(Duration.ofSeconds(30))
                .header("User-Agent", "Mozilla/5.0 (PageTitleFetcher/1.0)")
                .GET()
                .build();

        HttpResponse<String> response = client.send(request, HttpResponse.BodyHandlers.ofString());

        if (response.statusCode() >= 400) {
            throw new RuntimeException("HTTP error " + response.statusCode() + " for URL: " + url);
        }

        return response.body();
    }

    public static String extractTitle(String html) {
        if (html == null || html.isEmpty()) {
            return "";
        }

        Matcher matcher = TITLE_PATTERN.matcher(html);
        if (!matcher.find()) {
            return "";
        }

        String title = matcher.group(1);
        title = title.replaceAll("\\s+", " ").trim();
        title = title.replace("&amp;", "&")
                     .replace("&lt;", "<")
                     .replace("&gt;", ">")
                     .replace("&quot;", "\"")
                     .replace("&#39;", "'")
                     .replace("&apos;", "'")
                     .replace("&nbsp;", " ");
        return title;
    }

    public static String getPageTitle(String url) throws Exception {
        String html = fetchPageContent(url);
        return extractTitle(html);
    }

    public static void main(String[] args) {
        if (args == null || args.length == 0) {
            System.err.println("Usage: java PageTitleFetcher <https-url> [more-urls...]");
            System.exit(1);
        }

        for (String url : args) {
            try {
                String title = getPageTitle(url);
                System.out.println(url + " -> " + (title.isEmpty() ? "(no title found)" : title));
            } catch (Exception e) {
                System.err.println(url + " -> Error: " + e.getMessage());
            }
        }
    }
}