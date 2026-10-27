import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.nio.charset.StandardCharsets;
import java.security.KeyManagementException;
import java.security.NoSuchAlgorithmException;
import java.security.cert.Certificate;
import java.security.cert.X509Certificate;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import javax.net.ssl.SNIHostName;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLParameters;
import javax.net.ssl.SSLSession;
import javax.net.ssl.SSLSocket;
import javax.net.ssl.SSLSocketFactory;

public class SecureTlsClient {

    public static class TlsSessionInfo {
        private final String protocol;
        private final String cipherSuite;
        private final List<String> peerCertificates;
        private final String responsePayload;

        public TlsSessionInfo(String protocol, String cipherSuite, List<String> peerCertificates, String responsePayload) {
            this.protocol = protocol;
            this.cipherSuite = cipherSuite;
            this.peerCertificates = Collections.unmodifiableList(peerCertificates);
            this.responsePayload = responsePayload;
        }

        public String getProtocol() {
            return protocol;
        }

        public String getCipherSuite() {
            return cipherSuite;
        }

        public List<String> getPeerCertificates() {
            return peerCertificates;
        }

        public String getResponsePayload() {
            return responsePayload;
        }

        @Override
        public String toString() {
            StringBuilder sb = new StringBuilder();
            sb.append("TLS Protocol: ").append(protocol).append("\n");
            sb.append("Cipher Suite: ").append(cipherSuite).append("\n");
            sb.append("Peer Certificates (").append(peerCertificates.size()).append("):\n");
            for (String cert : peerCertificates) {
                sb.append("  - ").append(cert).append("\n");
            }
            sb.append("Response Payload:\n").append(responsePayload);
            return sb.toString();
        }
    }

    public static SSLContext initializeSslContext(String tlsVersion) throws NoSuchAlgorithmException, KeyManagementException {
        SSLContext sslContext = SSLContext.getInstance(tlsVersion != null ? tlsVersion : "TLS");
        sslContext.init(null, null, null);
        return sslContext;
    }

    public static SSLSocket createConfiguredSocket(
            SSLContext sslContext,
            String host,
            int port,
            int connectTimeoutMs,
            int readTimeoutMs,
            String[] enabledProtocols,
            String[] enabledCipherSuites) throws IOException {

        SSLSocketFactory factory = sslContext.getSocketFactory();
        SSLSocket socket = (SSLSocket) factory.createSocket();

        socket.connect(new java.net.InetSocketAddress(host, port), connectTimeoutMs);
        socket.setSoTimeout(readTimeoutMs);

        SSLParameters params = socket.getSSLParameters();
        params.setServerNames(Collections.singletonList(new SNIHostName(host)));
        params.setEndpointIdentificationAlgorithm("HTTPS");

        if (enabledProtocols != null && enabledProtocols.length > 0) {
            params.setProtocols(enabledProtocols);
        }
        if (enabledCipherSuites != null && enabledCipherSuites.length > 0) {
            params.setCipherSuites(enabledCipherSuites);
        }

        socket.setSSLParameters(params);
        return socket;
    }

    public static TlsSessionInfo executeTlsExchange(
            String host,
            int port,
            String requestPayload,
            int timeoutMs,
            String[] enabledProtocols,
            String[] enabledCipherSuites) throws IOException, NoSuchAlgorithmException, KeyManagementException {

        SSLContext sslContext = initializeSslContext("TLS");

        try (SSLSocket socket = createConfiguredSocket(
                sslContext, host, port, timeoutMs, timeoutMs, enabledProtocols, enabledCipherSuites);
             BufferedWriter writer = new BufferedWriter(new OutputStreamWriter(socket.getOutputStream(), StandardCharsets.UTF_8));
             BufferedReader reader = new BufferedReader(new InputStreamReader(socket.getInputStream(), StandardCharsets.UTF_8))) {

            socket.startHandshake();

            SSLSession session = socket.getSession();
            String negotiatedProtocol = session.getProtocol();
            String negotiatedCipher = session.getCipherSuite();

            List<String> certSummaries = new ArrayList<>();
            Certificate[] certificates = session.getPeerCertificates();
            for (Certificate cert : certificates) {
                if (cert instanceof X509Certificate) {
                    X509Certificate x509 = (X509Certificate) cert;
                    certSummaries.add(x509.getSubjectX500Principal().getName());
                } else {
                    certSummaries.add(cert.getType());
                }
            }

            if (requestPayload != null && !requestPayload.isEmpty()) {
                writer.write(requestPayload);
                writer.flush();
            }

            StringBuilder responseBuilder = new StringBuilder();
            String line;
            while ((line = reader.readLine()) != null) {
                responseBuilder.append(line).append("\n");
                if (line.isEmpty() && requestPayload != null && requestPayload.startsWith("HEAD")) {
                    break;
                }
            }

            return new TlsSessionInfo(negotiatedProtocol, negotiatedCipher, certSummaries, responseBuilder.toString());
        }
    }

    public static void main(String[] args) {
        String host = (args.length > 0) ? args[0] : "www.google.com";
        int port = (args.length > 1) ? Integer.parseInt(args[1]) : 443;
        int timeoutMs = 5000;

        String httpRequest = "HEAD / HTTP/1.1\r\n" +
                "Host: " + host + "\r\n" +
                "User-Agent: SecureTlsClient/1.0\r\n" +
                "Connection: close\r\n\r\n";

        String[] preferredProtocols = new String[]{"TLSv1.3", "TLSv1.2"};

        try {
            System.out.println("Connecting to " + host + ":" + port + " over TLS...");
            TlsSessionInfo sessionInfo = executeTlsExchange(host, port, httpRequest, timeoutMs, preferredProtocols, null);
            System.out.println("\nConnection successfully established. Session Details:");
            System.out.println(sessionInfo);
        } catch (Exception e) {
            System.err.println("TLS Connection failed: " + e.getMessage());
            e.printStackTrace();
        }
    }
}