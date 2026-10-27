import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.SocketAddress;
import java.net.UnknownHostException;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Locale;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class DnsServer {

    static final int TYPE_A = 1;
    static final int CLASS_IN = 1;
    static final int HEADER_LEN = 12;
    static final int MAX_PACKET = 4096;

    static final int RCODE_NOERROR = 0;
    static final int RCODE_FORMERR = 1;
    static final int RCODE_SERVFAIL = 2;
    static final int RCODE_NOTIMP = 4;

    static class DnsFormatException extends Exception {
        DnsFormatException(String msg) { super(msg); }
    }

    static class Query {
        int id;
        int flags;
        int opcode;
        int rd;
        String name;
        int type;
        int qclass;
        byte[] questionSection;
    }

    public static void main(String[] args) throws IOException {
        String bindIp = args.length > 0 ? args[0] : "127.0.0.1";
        int port = args.length