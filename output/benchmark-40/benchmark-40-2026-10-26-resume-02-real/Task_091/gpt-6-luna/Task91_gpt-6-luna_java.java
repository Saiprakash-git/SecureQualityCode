import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class DnsUdpServer {
    private static final int DNS_HEADER_SIZE = 12;
    private static final int MAX_UDP_DNS_SIZE = 512;
    private static final int TTL_SECONDS = 60;

    private static final class Question {
        final String name;
        final int nameOffset;
        final int type;
        final int dnsClass;

        Question(String name, int nameOffset, int type, int dnsClass) {
            this.name = name;
            this.nameOffset = nameOffset;
            this.type = type;
            this.dnsClass = dnsClass;
        }
    }

    private static final class ParsedName {
        final String name;
        final int nextOffset;

        ParsedName(String name, int nextOffset) {
            this.name = name;
            this.nextOffset = nextOffset;
        }
    }

    public static void main(String[] args) throws IOException {
        if (args.length != 2) {
            System.err.println("Usage: java DnsUdpServer <IP-address> <port>");
            return;
        }

        InetAddress bindAddress = InetAddress.getByName(args[0]);
        int port = Integer.parseInt(args[1]);
        if (port < 1 || port > 65535) {
            throw new IllegalArgumentException("Port must be between 1 and 65535");
        }

        try (DatagramSocket socket =
                     new DatagramSocket(new InetSocketAddress(bindAddress, port))) {
            byte[] receiveBuffer = new byte[65507];

            while (true) {
                DatagramPacket requestPacket =
                        new DatagramPacket(receiveBuffer, receiveBuffer.length);
                socket.receive(requestPacket);

                byte[] request = Arrays.copyOfRange(
                        requestPacket.getData(),
                        requestPacket.getOffset(),
                        requestPacket.getOffset() + requestPacket.getLength());

                byte[] response = handleQuery(request);
                if (response != null) {
                    DatagramPacket responsePacket = new DatagramPacket(
                            response,
                            response.length,
                            requestPacket.getAddress(),
                            requestPacket.getPort());
                    socket.send(responsePacket);
                }
            }
        }
    }

    /**
     * Parses the DNS header and question section, resolves supported records using
     * the host system's resolver, and builds a UDP DNS response.
     */
    public static byte[] handleQuery(byte[] request) {
        if (request == null || request.length < 2) {
            return null;
        }

        int transactionId = readU16(request, 0);
        if (request.length < DNS_HEADER_SIZE) {
            return errorResponse(transactionId, 1, 0);
        }

        int requestFlags = readU16(request, 2);
        int questionCount = readU16(request, 4);

        try {
            if ((requestFlags & 0x8000) != 0 || questionCount == 0) {
                return errorResponse(transactionId, 1, requestFlags);
            }

            List<Question> questions = new ArrayList<Question>();
            int offset = DNS_HEADER_SIZE;

            for (int i = 0; i < questionCount; i++) {
                int nameOffset = offset;
                ParsedName parsedName = parseName(request, offset);
                offset = parsedName.nextOffset;

                if (offset + 4 > request.length) {
                    throw new IllegalArgumentException("Incomplete DNS question");
                }

                int type = readU16(request, offset);
                int dnsClass = readU16(request, offset + 2);
                offset += 4;

                questions.add(new Question(parsedName.name, nameOffset, type, dnsClass));
            }

            byte[] questionSection = Arrays.copyOfRange(request, DNS_HEADER_SIZE, offset);
            if (DNS_HEADER_SIZE + questionSection.length > MAX_UDP_DNS_SIZE) {
                return errorResponse(transactionId, 1, requestFlags);
            }

            int opcode = (requestFlags >>> 11) & 0x0f;
            if (opcode != 0) {
                return buildResponse(transactionId, requestFlags, questionSection,
                        questions.size(), new ArrayList<byte[]>(), 4, false);
            }

            List<byte[]> answers = new ArrayList<byte[]>();
            boolean lookupAttempted = false;
            boolean lookupFailed = false;

            // Resolve A and AAAA questions through the operating system's DNS resolver.
            for (Question question : questions) {
                if (question.dnsClass != 1
                        || (question.type != 1 && question.type != 28 && question.type != 255)
                        || question.name.length() == 0) {
                    continue;
                }

                lookupAttempted = true;
                try {
                    InetAddress[] addresses = InetAddress.getAllByName(question.name);
                    for (InetAddress address : addresses) {
                        byte[] addressBytes = address.getAddress();
                        int recordType = addressBytes.length == 4 ? 1
                                : addressBytes.length == 16 ? 28 : 0;

                        if (recordType == 0
                                || (question.type != 255 && question.type != recordType)) {
                            continue;
                        }

                        answers.add(makeAddressRecord(question.nameOffset, recordType, addressBytes));
                    }
                } catch (UnknownHostException e) {
                    lookupFailed = true;
                }
            }

            int rcode = answers.isEmpty() && lookupAttempted && lookupFailed ? 3 : 0;
            return buildResponse(transactionId, requestFlags, questionSection,
                    questions.size(), answers, rcode, false);
        } catch (IllegalArgumentException e) {
            return errorResponse(transactionId, 1, requestFlags);
        }
    }

    private static byte[] makeAddressRecord(int nameOffset, int type, byte[] address) {
        ByteArrayOutputStream record = new ByteArrayOutputStream();
        if (nameOffset >= 0 && nameOffset < 0x4000) {
            writeU16(record, 0xC000 | nameOffset);
        } else {
            writeU16(record, 0xC00C);
        }
        writeU16(record, type);
        writeU16(record, 1);
        writeU32(record, TTL_SECONDS);
        writeU16(record, address.length);
        record.write(address, 0, address.length);
        return record.toByteArray();
    }

    private static byte[] buildResponse(
            int transactionId,
            int requestFlags,
            byte[] questionSection,
            int questionCount,
            List<byte[]> answers,
            int rcode,
            boolean initiallyTruncated) {

        ByteArrayOutputStream packet = new ByteArrayOutputStream();
        int includedAnswers = 0;
        boolean truncated = initiallyTruncated;

        writeU16(packet, transactionId);
        writeU16(packet, makeResponseFlags(requestFlags, rcode, truncated));
        writeU16(packet, questionCount);
        writeU16(packet, 0);
        writeU16(packet, 0);
        writeU16(packet, 0);
        packet.write(questionSection, 0, questionSection.length);

        for (byte[] answer : answers) {
            if (packet.size() + answer.length > MAX_UDP_DNS_SIZE) {
                truncated = true;
                break;
            }
            packet.write(answer, 0, answer.length);
            includedAnswers++;
        }

        byte[] result = packet.toByteArray();
        result[2] = (byte) (makeResponseFlags(requestFlags, rcode, truncated) >>> 8);
        result[3] = (byte) makeResponseFlags(requestFlags, rcode, truncated);
        result[6] = (byte) (includedAnswers >>> 8);
        result[7] = (byte) includedAnswers;
        return result;
    }

    private static byte[] errorResponse(int transactionId, int rcode, int requestFlags) {
        return buildResponse(
                transactionId,
                requestFlags,
                new byte[0],
                0,
                new ArrayList<byte[]>(),
                rcode,
                false);
    }

    private static int makeResponseFlags(int requestFlags, int rcode, boolean truncated) {
        int flags = 0x8000; // Response
        flags |= requestFlags & 0x7800; // Opcode
        flags |= requestFlags & 0x0100; // Recursion desired
        flags |= 0x0080; // Recursion available
        if (truncated) {
            flags |= 0x0200;
        }
        flags |= rcode & 0x000f;
        return flags;
    }

    private static ParsedName parseName(byte[] packet, int startOffset) {
        if (startOffset < 0 || startOffset >= packet.length) {
            throw new IllegalArgumentException("Invalid DNS name offset");
        }

        StringBuilder name = new StringBuilder();
        boolean[] visited = new boolean[packet.length];
        int position = startOffset;
        int nextOffset = -1;
        int wireLength = 1;

        while (true) {
            if (position < 0 || position >= packet.length || visited[position]) {
                throw new IllegalArgumentException("Invalid or cyclic DNS name");
            }
            visited[position] = true;

            int length = packet