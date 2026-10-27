import java.io.IOException;
import java.io.InputStream;
import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.StandardCopyOption;
import java.time.Duration;
import java.util.concurrent.TimeUnit;

public class DownloadAndRun {

    public static void main(String[] args) {
        if (args.length < 1) {
            System.err.println("Usage: java DownloadAndRun <exe-url>");
            System.exit(1);
        }
        try {
            URI uri = validateUrl(args[0]);
            Path downloaded = downloadFile(uri, tempDirectory());
            if (!isPortableExecutable(downloaded)) {
                throw new IOException("Downloaded file is not a valid Windows EXE: " + downloaded);
            }
            int exitCode = executeFile(downloaded, 0);
            System.out.println("Process exited with code: " + exitCode);
            System.exit(exitCode);
        } catch (Exception e) {
            System.err.println("Error: " + e.getMessage());
            System.exit(2);
        }
    }

    static URI validateUrl(String url) throws IllegalArgumentException {
        if (url == null || url.isBlank()) {
            throw new IllegalArgumentException("URL must not be empty");
        }
        URI uri = URI.create(url.trim());
        String scheme = uri.getScheme();
        if (scheme == null || !(scheme.equalsIgnoreCase("http") || scheme.equalsIgnoreCase("https"))) {
            throw new IllegalArgumentException("Only http and https URLs are supported");
        }
        String path = uri.getPath();
        if (path == null || !path.toLowerCase().endsWith(".exe")) {
            throw new IllegalArgumentException("URL must point to a .exe file");
        }
        return uri;
    }

    static Path tempDirectory() throws IOException {
        Path dir = Paths.get(System.getProperty("java.io.tmpdir"), "exe_downloads");
        Files.createDirectories(dir);
        return dir;
    }

    static Path downloadFile(URI uri, Path targetDir) throws IOException, InterruptedException {
        String rawName = Paths.get(uri.getPath()).getFileName().toString();
        String fileName = rawName.replaceAll("[^A-Za-z0-9._-]", "_");
        if (fileName.isEmpty()) {
            fileName = "download.exe";
        }
        Path target = targetDir.resolve(fileName);

        HttpClient client = HttpClient.newBuilder()
                .followRedirects(HttpClient.Redirect.NORMAL)
                .connectTimeout(Duration.ofSeconds(30))
                .build();

        HttpRequest request = HttpRequest.newBuilder(uri)
                .timeout(Duration.ofMinutes(10))
                .GET()
                .build();

        HttpResponse<InputStream> response = client.send(request, HttpResponse.BodyHandlers.ofInputStream());
        if (response.statusCode() != 200) {
            response.body().close();
            throw new IOException("Download failed with HTTP status " + response.statusCode());
        }
        try (InputStream in = response.body()) {
            Files.copy(in, target, StandardCopyOption.REPLACE_EXISTING);
        }
        return target;
    }

    static boolean isPortableExecutable(Path file) throws IOException {
        if (!Files.isRegularFile(file) || Files.size(file) < 2) {
            return false;
        }
        try (InputStream in = Files.newInputStream(file)) {
            int b1 = in.read();
            int b2 = in.read();
            return b1 == 'M' && b2 == 'Z';
        }
    }

    static int executeFile(Path exe, long timeoutMinutes) throws IOException, InterruptedException {
        ProcessBuilder pb = new ProcessBuilder(exe.toAbsolutePath().toString());
        pb.inheritIO();
        Process process = pb.start();
        if (timeoutMinutes > 0) {
            if (!process.waitFor(timeoutMinutes, TimeUnit.MINUTES)) {
                process.destroyForcibly();
                throw new IOException("Process timed out after " + timeoutMinutes + " minutes");
            }
        } else {
            process.waitFor();
        }
        return process.exitValue();
    }
}