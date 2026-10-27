import javax.net.ssl.*;
import java.io.*;
import java.net.InetSocketAddress;
import java.security.KeyStore;
import java.security.SecureRandom;
import java.security.cert.X509Certificate;
import java.util.Collection;
import java.util.List;

public class SecureSSLClient {

    public static SSLContext createSSLContext() throws Exception {
        return createSSLContext(null, null);
    }

    public static SSLContext createSSLContext(String trustStorePath, String trustStorePassword) throws Exception {
        TrustManager[] trustManagers = null;

        if (trustStorePath != null) {
            KeyStore trustStore = KeyStore.getInstance(KeyStore.getDefaultType());
            try (FileInputStream fis = new FileInputStream(trustStorePath)) {
                trustStore.load(fis, trustStorePassword != null ? trustStorePassword.toCharArray() : null);
            }
            TrustManagerFactory tmf = TrustManagerFactory.getInstance(TrustManagerFactory.getDefaultAlgorithm());
            tmf.init(trustStore);
            trustManagers = tmf.getTrustManagers();
        }

        SSLContext sslContext = SSLContext.getInstance("TLSv1.3");
        sslContext.init(null, trustManagers, new SecureRandom());
        return sslContext;
    }

    public static SSLSocket connect(String host, int port, SSLContext sslContext) throws IOException {
        SSLSocketFactory factory = sslContext.getSocketFactory();
        SSLSocket socket = (SSLSocket) factory.createSocket();

        try {
            socket.connect(new InetSocketAddress(host, port), 10000);
            socket.setSoTimeout(10000);
            socket.startHandshake();

            SSLSession session = socket.getSession();
            if (!verifyHostname(host, session)) {
                throw new SSLException("Hostname verification failed for: " + host);
            }

            return socket;
        } catch (Exception e) {
            try {
                socket.close();
            } catch (IOException ignored) {
            }
            throw e;
        }
    }

    public static SSLSocket connect(String host, int port) throws Exception {
        SSLContext sslContext = createSSLContext();
        return connect(host, port, sslContext);
    }

    public static boolean verifyHostname(String host, SSLSession session) {
        try {
            X509Certificate cert = (X509Certificate) session.getPeerCertificates()[0];
            String certCN = extractCN(cert.getSubjectX500Principal().getName());

            if (certCN.equals(host) || certCN.equals("*." + host) || host.endsWith(certCN.replace("*.", ""))) {
                return true;
            }

            Collection<List<?>> altNames = cert.getSubjectAlternativeNames();
            if (altNames != null) {
                for (List<?> altName : altNames) {
                    if (altName.size() >= 2) {
                        Integer type = (Integer) altName.get(0);
                        if (type == 2) {
                            String dnsName = (String) altName.get(1);
                            if (dnsName.equals(host) || dnsName.equals("*." + host)) {
                                return true;
                            }
                        }
                    }
                }
            }
        } catch (Exception e) {
            return false;
        }
        return false;
    }

    private static String extractCN(String principal) {
        String[] parts = principal.split(",");
        for (String part : parts) {
            part = part.trim();
            if (part.startsWith("CN=")) {
                return part.substring(3);
            }
        }
        return "";
    }

    public static void sendMessage(SSLSocket socket, String message) throws IOException {
        OutputStream out = socket.getOutputStream();
        PrintWriter writer = new PrintWriter(new OutputStreamWriter(out, "UTF-8"), true);
        writer.println(message);
        writer.flush();
    }

    public static String receiveMessage(SSLSocket socket) throws IOException {
        InputStream in = socket.getInputStream();
        BufferedReader reader = new BufferedReader(new InputStreamReader(in, "UTF-8"));
        return reader.readLine();
    }

    public static String sendAndReceive(String host, int port, String message) throws Exception {
        SSLSocket socket = null;
        try {
            socket = connect(host, port);
            sendMessage(socket, message);
            return receiveMessage(socket);
        } finally {
            if (socket != null) {
                socket.close();
            }
        }
    }

    public static void closeConnection(SSLSocket socket) throws IOException {
        if (socket != null && !socket.isClosed()) {
            socket.close();
        }
    }

    public static void main(String[] args) {
        try {
            String host = args.length > 0 ? args[0] : "www.google.com";
            int port = args.length > 1 ? Integer.parseInt(args[1]) : 443;
            String message = args.length > 2 ? args[2] : "GET / HTTP/1.1\r\nHost: " + host + "\r\nConnection: close\r\n\r\n";

            System.out.println("Connecting to " + host + ":" + port + " via SSL/TLS...");

            SSLSocket socket = connect(host, port);
            System.out.println("Connected. Cipher: " + socket.getSession().getCipherSuite());
            System.out.println("Protocol: " + socket.getSession().getProtocol());

            sendMessage(socket, message);

            String response;
            while ((response = receiveMessage(socket)) != null) {
                System.out.println(response);
            }

            closeConnection(socket);
            System.out.println("Connection closed.");

        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}