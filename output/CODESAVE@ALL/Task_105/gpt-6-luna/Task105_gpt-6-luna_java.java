import java.io.IOException;
import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.security.GeneralSecurityException;
import java.time.Duration;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLParameters;

public final class SecureTlsClient {
    private SecureTlsClient() {
    }

    public static HttpResponse<String> get(String url, Duration timeout)
            throws IOException, InterruptedException, GeneralSecurityException {
        if (url == null || timeout == null || timeout.isZero() || timeout.isNegative()) {
            throw new IllegalArgumentException("A URL and a positive timeout are required.");
        }

        URI uri = URI.create(url);
        if (!"https".equalsIgnoreCase(uri.getScheme())
                || uri.getHost() == null
                || uri.getUserInfo() != null) {
            throw new IllegalArgumentException("The URL must be an HTTPS URL without user information.");
        }

        SSLParameters sslParameters = new SSLParameters();
        sslParameters.setProtocols(new String[] {"TLSv1.3", "TLSv1.2"});
        sslParameters.setEndpointIdentificationAlgorithm("HTTPS");

        HttpClient client = HttpClient.newBuilder()
                .sslContext(SSLContext.getDefault())
                .sslParameters(sslParameters)
                .connectTimeout(timeout)
                .followRedirects(HttpClient.Redirect.NEVER)
                .build();

        HttpRequest request = HttpRequest.newBuilder(uri)
                .timeout(timeout)
                .GET()
                .build();

        return client.send(request, HttpResponse.BodyHandlers.ofString());
    }

    public static void main(String[] args) throws Exception {
        if (args.length != 1) {
            throw new IllegalArgumentException("Usage: java SecureTlsClient <https-url>");
        }

        HttpResponse<String> response = get(args[0], Duration.ofSeconds(30));
        System.out.println("HTTP status: " + response.statusCode());
        System.out.print(response.body());
    }
}