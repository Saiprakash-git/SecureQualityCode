import java.io.IOException;
import java.io.InputStream;
import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.security.MessageDigest;
import java.time.Duration;
import java.util.HexFormat;

public final class SafeExeDownloader {
    private static final long MAX_DOWNLOAD_BYTES = 250L * 1024L * 1024L;

    private SafeExeDownloader() {
    }

    public static void main(String[] args) {
        if (args.length != 3) {
            System.err.println("Usage: java SafeExeDownloader <https-url> <expected-sha256> <output-file>");
            System.exit(2);
        }

        try {
            Path downloadedFile = downloadVerified(args[0], args[1], Path.of(args[2]));
            System.out.println("Downloaded and verified: " + downloadedFile.toAbsolutePath());
            System.out.println("The file was not executed. Verify its source and run it manually only if trusted.");
        } catch (Exception e) {
            System.err.println("Download failed: " + e.getMessage());
            System.exit(1);
        }
    }

    public static Path downloadVerified(String url, String expectedSha256, Path destination)
            throws Exception {
        URI uri = URI.create(url);
        validateHttpsUri(uri);

        String normalizedHash = normalizeSha256(expectedSha256);
        Path absoluteDestination = destination.toAbsolutePath().normalize();
        Path parent = absoluteDestination.getParent();

        if (parent == null) {
            throw new IOException("Output path must have a parent directory");
        }

        Files.createDirectories(parent);
        Path temporaryFile = Files.createTempFile(parent, ".download-", ".tmp");
        boolean moved = false;

        try {
            HttpClient client = HttpClient.newBuilder()
                    .connectTimeout(Duration.ofSeconds(20))
                    .followRedirects(HttpClient.Redirect.NEVER)
                    .build();

            HttpRequest request = HttpRequest.newBuilder(uri)
                    .timeout(Duration.ofMinutes(5))
                    .header("User-Agent", "SafeExeDownloader/1.0")
                    .GET()
                    .build();

            HttpResponse<InputStream> response =
                    client.send(request, HttpResponse.BodyHandlers.ofInputStream());

            try (InputStream input = response.body()) {
                if (response.statusCode() != 200) {
                    throw new IOException("Server returned HTTP status " + response.statusCode());
                }

                long contentLength = response.headers()
                        .firstValueAsLong("Content-Length")
                        .orElse(-1L);
                if (contentLength > MAX_DOWNLOAD_BYTES) {
                    throw new IOException("Download exceeds the maximum allowed size");
                }

                copyWithLimit(input, temporaryFile, MAX_DOWNLOAD_BYTES);
            }

            String actualHash = sha256(temporaryFile);
            if (!actualHash.equals(normalizedHash)) {
                throw new IOException("SHA-256 mismatch; downloaded file was rejected");
            }

            try {
                Files.move(temporaryFile, absoluteDestination, StandardCopyOption.ATOMIC_MOVE,
                        StandardCopyOption.REPLACE_EXISTING);
            } catch (IOException atomicMoveUnavailable) {
                Files.move(temporaryFile, absoluteDestination, StandardCopyOption.REPLACE_EXISTING);
            }

            moved = true;
            return absoluteDestination;
        } finally {
            if (!moved) {
                Files.deleteIfExists(temporaryFile);
            }
        }
    }

    private static void validateHttpsUri(URI uri) throws IOException {
        if (!"https".equalsIgnoreCase(uri.getScheme())
                || uri.getHost() == null
                || uri.getUserInfo() != null) {
            throw new IOException("Only HTTPS URLs without embedded credentials are accepted");
        }
    }

    private static String normalizeSha256(String hash) throws IOException {
        if (hash == null || !hash.matches("(?i)[0-9a-f]{64}")) {
            throw new IOException("Expected SHA-256 must contain exactly 64 hexadecimal characters");
        }
        return hash.toLowerCase();
    }

    private static void copyWithLimit(InputStream input, Path destination, long limit)
            throws IOException {
        byte[] buffer = new byte[8192];
        long total = 0;

        try (var output = Files.newOutputStream(destination)) {
            int count;
            while ((count = input.read(buffer)) != -1) {
                total += count;
                if (total > limit) {
                    throw new IOException("Download exceeds the maximum allowed size");
                }
                output.write(buffer, 0, count);
            }
        }
    }

    private static String sha256(Path file) throws Exception {
        MessageDigest digest = MessageDigest.getInstance("SHA-256");
        try (InputStream input = Files.newInputStream(file)) {
            byte[] buffer = new byte[8192];
            int count;
            while ((count = input.read(buffer)) != -1) {
                digest.update(buffer, 0, count);
            }
        }
        return HexFormat.of().formatHex(digest.digest());
    }
}