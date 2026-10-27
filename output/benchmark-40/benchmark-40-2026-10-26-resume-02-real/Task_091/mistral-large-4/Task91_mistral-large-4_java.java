import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.nio.ByteBuffer;
import java.util.Arrays;

public class DnsServer {

    public static void main(String[] args) {
        String ip = args.length > 0 ? args[0] : "0.0.0.0";
        int port = args.length > 1 ? Integer.parseInt(args[1]) : 5353;
        startServer(ip, port);
    }

    public static void startServer(String ipAddress, int port) {
        try (DatagramSocket socket = new DatagramSocket(port, InetAddress.getByName(ipAddress))) {
            byte[] buffer = new byte[512];
            while (true) {
                DatagramPacket request = new DatagramPacket(buffer, buffer.length);
                socket.receive(request);
                byte[] queryData = Arrays.copyOf(request.getData(), request.getLength());
                byte[] responseData = parseAndResolve(queryData);
                if (responseData != null) {
                    sendResponse(socket, request.getAddress(), request.getPort(), responseData);
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public static byte[] parseAndResolve(byte[] queryData) {
        try {
            ByteBuffer query = ByteBuffer.wrap(queryData);
            int transactionId = query.getShort() & 0xFFFF;
            query.getShort();
            int questionCount = query.getShort() & 0xFFFF;
            query.getShort();
            query.getShort();
            query.getShort();

            StringBuilder domain = new StringBuilder();
            byte len;
            while ((len = query.get()) != 0) {
                byte[] label = new byte[len];
                query.get(label);
                if (domain.length() > 0) domain.append(".");
                domain.append(new String(label));
            }

            int queryType = query.getShort() & 0xFFFF;
            int queryClass = query.getShort() & 0xFFFF;

            ByteBuffer response = ByteBuffer.allocate(512);
            response.putShort((short) transactionId);
            response.putShort((short) 0x8180);
            response.putShort((short) 1);
            response.putShort((short) 1);
            response.putShort((short) 0);
            response.putShort((short) 0);

            String[] labels = domain.toString().split("\\.");
            for (String label : labels) {
                response.put((byte) label.length());
                response.put(label.getBytes());
            }
            response.put((byte) 0);
            response.putShort((short) queryType);
            response.putShort((short) queryClass);

            response.putShort((short) 0xC00C);
            response.putShort((short) 1);
            response.putShort((short) 1);
            response.putInt(300);
            response.putShort((short) 4);

            byte[] ipBytes;
            try {
                ipBytes = InetAddress.getByName(domain.toString()).getAddress();
            } catch (Exception e) {
                ipBytes = new byte[]{127, 0, 0, 1};
            }
            response.put(ipBytes);

            return Arrays.copyOf(response.array(), response.position());
        } catch (Exception e) {
            return null;
        }
    }

    public static void sendResponse(DatagramSocket socket, InetAddress address, int port, byte[] data) {
        try {
            DatagramPacket packet = new DatagramPacket(data, data.length, address, port);
            socket.send(packet);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}