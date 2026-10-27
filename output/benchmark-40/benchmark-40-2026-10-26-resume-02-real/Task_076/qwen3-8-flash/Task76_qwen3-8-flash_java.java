import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class Main {
    private static final Pattern TITLE_PATTERN = Pattern.compile("(?is)<title[^>]*>(.*?)</title>");

    public static String getPageTitle(String urlString) throws IOException {
        URL url = new URL(urlString);
        String protocol = url.getProtocol();

        if (protocol == null || (!protocol.equalsIgnoreCase("http") && !protocol.equalsIgnoreCase("https"))) {
            throw new IllegalArgumentException("URL must use HTTP or HTTPS");
        }

        int redirects = 0;

        while (redirects <= 5) {
            HttpURLConnection connection = (HttpURLConnection) url.openConnection();
            connection.setRequestMethod("GET");
            connection.setInstanceFollowRedirects(false);
            connection.setConnectTimeout(10000);
            connection.setReadTimeout(10000);
            connection.setRequestProperty("User-Agent", "Java-Title-Extractor/1.0");
            connection.connect();

            int status = connection.getResponseCode();

            if (status >= 300 && status < 400) {
                String location = connection.getHeaderField("Location");
                connection.disconnect();

                if (location == null || location.isEmpty()) {
                    throw new IOException("Redirect without Location header");
                }

                url = new URL(url, location);
                redirects++;
                continue;
            }

            if (status < 200 || status >= 300) {
                connection.disconnect();
                throw new IOException("HTTP error: " + status);
            }

            InputStream input = connection.getInputStream();
            ByteArrayOutputStream output = new ByteArrayOutputStream();
            byte[] buffer = new byte[4096];
            int read;

            while ((read = input.read(buffer)) != -1) {
                output.write(buffer, 0, read);
            }

            input.close();
            connection.disconnect();

            String html = new String(output.toByteArray(), StandardCharsets.UTF_8);
            Matcher matcher = TITLE_PATTERN.matcher(html);

            if (matcher.find()) {
                return matcher.group(1).trim();
            }

            return "";
        }

        throw new IOException("Too many redirects");
    }

    public static void main(String[] args) {
        if (args.length == 0) {
            System.err.println("Usage: java Main <HTTPS_URL>");
            return;
        }

        try {
            String title = getPageTitle(args[0]);
            System.out.println(title);
        } catch (Exception e) {
            System.err.println(e.getMessage());
        }
    }
}