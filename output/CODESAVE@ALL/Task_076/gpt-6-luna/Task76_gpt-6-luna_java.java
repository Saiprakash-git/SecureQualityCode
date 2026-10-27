import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.Charset;
import java.nio.charset.StandardCharsets;
import java.util.HashMap;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class Main {
    private static final Pattern TITLE_PATTERN =
            Pattern.compile("<title(?:\\s[^>]*)?>(.*?)</title\\s*>", Pattern.CASE_INSENSITIVE | Pattern.DOTALL);
    private static final Pattern CHARSET_PATTERN =
            Pattern.compile("charset\\s*=\\s*[\"']?([^\\s;\"']+)", Pattern.CASE_INSENSITIVE);
    private static final Pattern ENTITY_PATTERN =
            Pattern.compile("&(#(?:x[0-9a-fA-F]+|[0-9]+)|[A-Za-z][A-Za-z0-9]+);");

    public static String getPageTitle(String url) throws IOException {
        HttpURLConnection connection = (HttpURLConnection) new URL(url).openConnection();
        connection.setRequestMethod("GET");
        connection.setInstanceFollowRedirects(true);
        connection.setConnectTimeout(15000);
        connection.setReadTimeout(15000);
        connection.setRequestProperty("User-Agent", "Mozilla/5.0");

        try {
            byte[] content;
            try (InputStream input = connection.getInputStream();
                 ByteArrayOutputStream output = new ByteArrayOutputStream()) {
                byte[] buffer = new byte[8192];
                int count;
                while ((count = input.read(buffer)) != -1) {
                    output.write(buffer, 0, count);
                }
                content = output.toByteArray();
            }

            Charset charset = getCharset(connection.getContentType());
            String html = new String(content, charset);
            Matcher matcher = TITLE_PATTERN.matcher(html);
            if (!matcher.find()) {
                return "";
            }

            String title = matcher.group(1).replaceAll("(?s)<!--.*?-->", "");
            title = title.replaceAll("(?s)<[^>]*>", "");
            return decodeEntities(title).trim();
        } finally {
            connection.disconnect();
        }
    }

    private static Charset getCharset(String contentType) {
        if (contentType != null) {
            Matcher matcher = CHARSET_PATTERN.matcher(contentType);
            if (matcher.find()) {
                try {
                    return Charset.forName(matcher.group(1));
                } catch (IllegalArgumentException ignored) {
                    // Fall back to UTF-8 for an unknown charset.
                }
            }
        }
        return StandardCharsets.UTF_8;
    }

    private static String decodeEntities(String text) {
        Map<String, String> namedEntities = new HashMap<>();
        namedEntities.put("amp", "&");
        namedEntities.put("lt", "<");
        namedEntities.put("gt", ">");
        namedEntities.put("quot", "\"");
        namedEntities.put("apos", "'");
        namedEntities.put("nbsp", "\u00A0");
        namedEntities.put("copy", "\u00A9");
        namedEntities.put("reg", "\u00AE");
        namedEntities.put("trade", "\u2122");
        namedEntities.put("mdash", "\u2014");
        namedEntities.put("ndash", "\u2013");
        namedEntities.put("hellip", "\u2026");
        namedEntities.put("rsquo", "\u2019");
        namedEntities.put("lsquo", "\u2018");
        namedEntities.put("rdquo", "\u201D");
        namedEntities.put("ldquo", "\u201C");

        Matcher matcher = ENTITY_PATTERN.matcher(text);
        StringBuffer result = new StringBuffer();
        while (matcher.find()) {
            String entity = matcher.group(1);
            String replacement = null;
            if (entity.startsWith("#")) {
                try {
                    int codePoint = entity.startsWith("#x") || entity.startsWith("#X")
                            ? Integer.parseInt(entity.substring(2), 16)
                            : Integer.parseInt(entity.substring(1));
                    if (Character.isValidCodePoint(codePoint)) {
                        replacement = new String(Character.toChars(codePoint));
                    }
                } catch (NumberFormatException ignored) {
                    // Leave malformed numeric entities unchanged.
                }
            } else {
                replacement = namedEntities.get(entity);
            }

            matcher.appendReplacement(result, Matcher.quoteReplacement(
                    replacement == null ? matcher.group() : replacement));
        }
        matcher.appendTail(result);
        return result.toString();
    }

    public static void main(String[] args) throws IOException {
        if (args.length == 0) {
            return;
        }
        System.out.print(getPageTitle(args[0]));
    }
}