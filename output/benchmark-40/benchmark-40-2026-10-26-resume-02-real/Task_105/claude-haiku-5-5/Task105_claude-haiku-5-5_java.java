import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLSocket;
import javax.net.ssl.SSLSocketFactory;
import javax.net.ssl.SSLParameters;
import javax.net.ssl.SSLSession;
import javax.net.ssl.TrustManagerFactory;
import javax.net.ssl.HttpsURLConnection;
import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.PrintWriter;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.security.KeyStore;
import java.security.cert.Certificate;
import java.security.cert.X509Certificate;
import java.util.ArrayList;
import java.util.List;

public class SecureTlsClient {

    public static class TlsResult {
        public final String protocol;
        public final String cipherSuite;
        public final String peerPrincipal;
        public final List<String> certificateSubjects;
        public final int responseCode;
        public final String responseBody;

        public TlsResult(String protocol, String cipherSuite, String peerPrincipal,
                         List<String> certificateSubjects, int responseCode, String responseBody) {
            this.protocol = protocol;
            this.cipherSuite = cipherSuite;
            this.peerPrincipal = peerPrincipal;
            this.certificateSubjects = certificateSubjects;
            this.responseCode = responseCode;
            this.responseBody = responseBody;
        }

        @Override
        public String toString() {
            StringBuilder sb = new StringBuilder();
            sb.append("Protocol: ").append(protocol).append("\n");
            sb.append("Cipher Suite: ").append(cipherSuite).append("\n");
            sb.append("Peer Principal: ").append(peerPrincipal).append("\n");
            sb.append("Certificate Chain:\n");
            for (String s : certificateSubjects) {
                sb.append("  - ").append(s).append("\n");
            }
            sb.append("HTTP Status: ").append(responseCode).append("\n");
            if (responseBody != null) {
                sb.append("Body (truncated):\n");
                sb.append(responseBody.length() > 500 ? responseBody.substring(0, 500) + "..." : responseBody);
                sb.append("\n");
            }
            return sb.toString();
        }
    }

    public static SSLContext createSecureContext(String protocol, KeyStore trustStore) throws Exception {
        SSLContext ctx = SSLContext.getInstance(protocol);
        TrustManagerFactory tmf = TrustManagerFactory.getInstance(TrustManagerFactory.getDefaultAlgorithm());
        tmf.init(trustStore);
        ctx.init(null, tmf.getTrustManagers(), null);
        return ctx;
    }

    public static TlsResult connectRaw(String host, int port, String sniHost, int timeoutMillis,
                                       String requestPath, KeyStore trustStore) throws Exception {
        SSLContext ctx = (trustStore == null)
                ? SSLContext.getDefault()
                : createSecureContext("TLSv1.3", trustStore);

        SSLSocketFactory factory = ctx.getSocketFactory();
        try (SSLSocket socket = (SSLSocket) factory.createSocket()) {
            socket.connect(new java.net.InetSocketAddress(host, port), timeoutMillis);
            socket.setSoTimeout(timeoutMillis);

            SSLParameters params = socket.getSSLParameters();
            params.setEndpointIdentificationAlgorithm("HTTPS");
            params.setProtocols(new String[]{"TLSv1.3", "TLSv1.2"});
            if (sniHost != null && !sniHost.isEmpty()) {
                params.setServerNames(java.util.Collections.singletonList(
                        new javax.net.ssl.SNIHostName(sniHost)));
            }
            socket.setSSLParameters(params);

            socket.startHandshake();

            SSLSession session = socket.getSession();
            String protocol = session.getProtocol();
            String cipher = session.getCipherSuite();
            String peer = session.getPeerPrincipal().getName();

            List<String> subjects = new ArrayList<>();
            for (Certificate c : session.getPeerCertificates()) {
                if (c instanceof X509Certificate) {
                    subjects.add(((X509Certificate) c).getSubjectX500Principal().getName());
                }
            }

            String path = (requestPath == null || requestPath.isEmpty()) ? "/" : requestPath;
            String hostHeader = (sniHost != null && !sniHost.isEmpty()) ? sniHost : host;
            OutputStream out = socket.getOutputStream();
            PrintWriter writer = new PrintWriter(new java.io.OutputStreamWriter(out, StandardCharsets.UTF_8), false);
            writer.print("GET " + path + " HTTP/1.1\r\n");
            writer.print("Host: " + hostHeader + "\r\n");
            writer.print("User-Agent: SecureTlsClient/1.0\r\n");
            writer.print("Connection: close\r\n\r\n");
            writer.flush();

            InputStream in = socket.getInputStream();
            BufferedReader reader = new BufferedReader(new InputStreamReader(in, StandardCharsets.UTF_8));
            String statusLine = reader.readLine();
            int status = -1;
            if (statusLine != null) {
                String[] parts = statusLine.split(" ");
                if (parts.length >= 2) {
                    try {
                        status = Integer.parseInt(parts[1]);
                    } catch (NumberFormatException ignored) {
                    }
                }
            }
            String line;
            while ((line = reader.readLine()) != null && !line.isEmpty()) {
                // skip headers
            }
            StringBuilder body = new StringBuilder();
            char[] buf = new char[1024];
            int n;
            while (body.length() < 4096 && (n = reader.read(buf)) != -1) {
                body.append(buf, 0, n);
            }

            return new TlsResult(protocol, cipher, peer, subjects, status, body.toString());
        }
    }

    public static TlsResult connectHttps(String urlString, int timeoutMillis) throws Exception {
        URL url = new URL(urlString);
        if (!"https".equalsIgnoreCase(url.getProtocol())) {
            throw new IllegalArgumentException("URL must use https scheme");
        }
        HttpsURLConnection conn = (HttpsURLConnection) url.openConnection();
        conn.setConnectTimeout(timeoutMillis);
        conn.setReadTimeout(timeoutMillis);
        conn.setRequestMethod("GET");
        conn.setRequestProperty("User-Agent", "SecureTlsClient/1.0");
        conn.connect();

        SSLSession session = conn.getSSLSession().orElseThrow(
                () -> new IllegalStateException("No SSL session established"));

        String protocol = session.getProtocol();
        String cipher = session.getCipherSuite();
        String peer = session.getPeerPrincipal().getName();

        List<String> subjects = new ArrayList<>();
        for (Certificate c : session.getPeerCertificates()) {
            if (c instanceof X509Certificate) {
                subjects.add(((X509Certificate) c).getSubjectX500Principal().getName());
            }
        }

        int status = conn.getResponseCode();
        StringBuilder body = new StringBuilder();
        InputStream is = (status >= 400) ? conn.getErrorStream() : conn.getInputStream();
        if (is != null) {
            try (BufferedReader reader = new BufferedReader(new InputStreamReader(is, StandardCharsets.UTF_8))) {
                char[] buf = new char[1024];
                int n;
                while (body.length() < 4096 && (n = reader.read(buf)) != -1) {
                    body.append(buf, 0, n);
                }
            }
        }
        conn.disconnect();
        return new TlsResult(protocol, cipher, peer, subjects, status, body.toString());
    }

    public static void main(String[] args) {
        String target = (args.length > 0) ? args[0] : "https://www.example.com/";
        try {
            TlsResult result = connectHttps(target, 10000);
            System.out.println(result);
        } catch (Exception e) {
            System.err.println("TLS connection failed: " + e.getMessage());
            e.printStackTrace();
            System.exit(1);
        }
    }
}