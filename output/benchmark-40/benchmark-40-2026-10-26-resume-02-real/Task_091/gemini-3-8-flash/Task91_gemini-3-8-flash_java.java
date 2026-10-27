import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Map;

public class SimpleDnsServer {

    private static final int DEFAULT_PORT = 5353;
    private static final String DEFAULT_HOST = "0.0.0.0";
    private static final int BUFFER_SIZE = 512;

    // In-memory DNS record store (Domain Name -> IPv4 Address)
    private final Map<String, byte[]> dnsRecords = new HashMap<>();

    public SimpleDnsServer() {
        // Sample records for demonstration
        dnsRecords.put("example.com", new byte[]{(byte) 93, (byte) 184, (byte) 216, (byte) 34});
        dnsRecords.put("localhost", new byte[]{(byte) 127, (byte) 0, (byte) 0, (byte) 1});
        dnsRecords.put("myrouter.local", new byte[]{(byte) 192, (byte) 168, (byte) 1, (byte) 1});
    }

    public static void main(String[] args) {
        String host = DEFAULT_HOST;
        int port = DEFAULT_PORT;

        if (args.length >= 1) {
            host = args[0];
        }
        if (args.length >= 2) {
            port = Integer.parseInt(args[1]);
        }

        SimpleDnsServer server = new SimpleDnsServer();
        server.start(host, port);
    }

    /**
     * Starts the UDP server loop on the specified IP and port.
     */
    public void start(String bindIp, int port) {
        try {
            InetAddress bindAddress = InetAddress.getByName(bindIp);
            try (DatagramSocket socket = new DatagramSocket(port, bindAddress)) {
                System.out.println("DNS Server listening on " + bindAddress.getHostAddress() + ":" + port);

                byte[] receiveBuffer = new byte[BUFFER_SIZE];

                while (!socket.isClosed()) {
                    DatagramPacket requestPacket = new DatagramPacket(receiveBuffer, receiveBuffer.length);
                    
                    // STEP 1: Listen and receive incoming UDP packet
                    socket.receive(requestPacket);

                    InetAddress clientAddress = requestPacket.getAddress();
                    int clientPort = requestPacket.getPort();

                    try {
                        // STEP 2: Parse incoming DNS request
                        DnsQuery query = parseDnsQuery(requestPacket.getData(), requestPacket.getLength());

                        // STEP 3: Resolve DNS record
                        byte[] resolvedIp = resolveRecord(query.qName, query.qType);

                        // STEP 4: Build DNS response packet
                        byte[] responseBytes = buildDnsResponse(query, resolvedIp);

                        // STEP 5: Send response back to requester over UDP
                        sendResponse(socket, responseBytes, clientAddress, clientPort);

                    } catch (Exception e) {
                        System.err.println("Failed to process request: " + e.getMessage());
                    }
                }
            }
        } catch (IOException e) {
            System.err.println("Socket error: " + e.getMessage());
        }
    }

    /**
     * STEP 2: Parses the received raw byte array into DNS header and question structures.
     */
    private DnsQuery parseDnsQuery(byte[] data, int length) throws IOException {
        if (length < 12) {
            throw new IOException("Packet too small to be a DNS query");
        }

        ByteBuffer buffer = ByteBuffer.wrap(data, 0, length);

        int id = buffer.getShort() & 0xFFFF;
        int flags = buffer.getShort() & 0xFFFF;
        int qdCount = buffer.getShort() & 0xFFFF;
        int anCount = buffer.getShort() & 0xFFFF;
        int nsCount = buffer.getShort() & 0xFFFF;
        int arCount = buffer.getShort() & 0xFFFF;

        if (qdCount < 1) {
            throw new IOException("No question found in query");
        }

        int questionStartOffset = buffer.position();

        // Parse QNAME
        StringBuilder domainName = new StringBuilder();
        while (buffer.hasRemaining()) {
            int len = buffer.get() & 0xFF;
            if (len == 0) {
                break;
            }
            if ((len & 0xC0) == 0xC0) {
                // Compression pointer encountered
                buffer.get();
                break;
            }
            for (int i = 0; i < len; i++) {
                domainName.append((char) buffer.get());
            }
            domainName.append(".");
        }

        if (domainName.length() > 0 && domainName.charAt(domainName.length() - 1) == '.') {
            domainName.setLength(domainName.length() - 1);
        }

        int qType = buffer.getShort() & 0xFFFF;
        int qClass = buffer.getShort() & 0xFFFF;
        int questionEndOffset = buffer.position();

        byte[] rawQuestion = new byte[questionEndOffset - questionStartOffset];
        System.arraycopy(data, questionStartOffset, rawQuestion, 0, rawQuestion.length);

        return new DnsQuery(id, flags, domainName.toString(), qType, qClass, rawQuestion);
    }

    /**
     * STEP 3: Resolves domain name to IP address.
     */
    private byte[] resolveRecord(String domainName, int qType) {
        // qType 1 corresponds to Type A (Host Address)
        if (qType == 1) {
            return dnsRecords.get(domainName.toLowerCase());
        }
        return null;
    }

    /**
     * STEP 4: Formats and constructs the DNS response frame (Header, Question, Answer).
     */
    private byte[] buildDnsResponse(DnsQuery query, byte[] resolvedIp) throws IOException {
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        DataOutputStream out = new DataOutputStream(baos);

        // Header: Transaction ID
        out.writeShort(query.id);

        // Header: Flags
        int qr = 1;         // Response flag (1 = response)
        int opcode = 0;     // Standard query
        int aa = 1;         // Authoritative answer
        int tc = 0;         // Not truncated
        int rd = (query.flags >> 8) & 0x01; // Echo recursion desired
        int ra = 1;         // Recursion available
        int z = 0;          // Reserved
        int rcode = (resolvedIp != null) ? 0 : 3; // 0 = No Error, 3 = NXDOMAIN

        int responseFlags = (qr << 15) | (opcode << 11) | (aa << 10) | (tc << 9)
                | (rd << 8) | (ra << 7) | (z << 4) | rcode;
        out.writeShort(responseFlags);

        // Header: Counts
        out.writeShort(1); // QDCOUNT: 1 question
        out.writeShort(resolvedIp != null ? 1 : 0); // ANCOUNT: 1 answer if resolved
        out.writeShort(0); // NSCOUNT: 0 authority records
        out.writeShort(0); // ARCOUNT: 0 additional records

        // Question Section: Echo the original question
        out.write(query.rawQuestion);

        // Answer Section: Written only if the record was resolved
        if (resolvedIp != null) {
            // Pointer to question name at byte 12 (0xC00C)
            out.writeShort(0xC00C);
            out.writeShort(1);           // TYPE: A
            out.writeShort(1);           // CLASS: IN
            out.writeInt(300);           // TTL: 300 seconds
            out.writeShort(resolvedIp.length); // RDLENGTH: 4 bytes for IPv4
            out.write(resolvedIp);       // RDATA: 4-byte IP address
        }

        out.flush();
        return baos.toByteArray();
    }

    /**
     * STEP 5: Transmits the formulated response back over UDP to the client.
     */
    private void sendResponse(DatagramSocket socket, byte[] responseData,
                              InetAddress clientAddress, int clientPort) throws IOException {
        DatagramPacket responsePacket = new DatagramPacket(
                responseData,
                responseData.length,
                clientAddress,
                clientPort
        );
        socket.send(responsePacket);
    }

    /**
     * Inner helper structure encapsulating parsed DNS query information.
     */
    private static class DnsQuery {
        final int id;
        final int flags;
        final String qName;
        final int qType;
        final int qClass;
        final byte[] rawQuestion;

        DnsQuery(int id, int flags, String qName, int qType, int qClass, byte[] rawQuestion) {
            this.id = id;
            this.flags = flags;
            this.qName = qName;
            this.qType = qType;
            this.qClass = qClass;
            this.rawQuestion = rawQuestion;
        }
    }
}