import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetSocketAddress;
import java.net.SocketTimeoutException;
import java.nio.charset.StandardCharsets;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLException;
import javax.net.ssl.SSLParameters;
import javax.net.ssl.SSLSession;
import javax.net.ssl.SSLSocket;
import javax.net.ssl.SSLSocketFactory;

public class Main {
    private static final String DEFAULT_HOST = "example.com";
    private static final int DEFAULT_PORT = 443;
    private static final int DEFAULT_TIMEOUT_MILLIS = 10000;
    private static final int READ_BUFFER_SIZE = 8192;

    public static void main(String[] args) {
        String host = args.length > 0 ? args[0] : DEFAULT_HOST;
        int port = args.length > 1 ? Integer.parseInt(args[1]) : DEFAULT_PORT;
        String request = args.length > 2 ? args[2] : null;
        int timeoutMillis = args.length > 3 ? Integer.parseInt(args[3]) : DEFAULT_TIMEOUT_MILLIS;

        try {
            String result = performSecureRequest(host, port, request, timeoutMillis);
            System.out.println(result);
        } catch (Exception exception) {
            System.err.println("Secure connection failed: " + exception.getClass().getName() + ": " + exception.getMessage());
        }