import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.math.BigInteger;
import java.nio.charset.StandardCharsets;
import java.security.KeyPair;
import java.security.KeyPairGenerator;
import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;
import java.security.interfaces.RSAPrivateCrtKey;
import java.security.interfaces.RSAPublicKey;
import java.util.Base64;

public class OpenSSHRSAKeyGenerator {

    public static String generateOpenSSHPrivateKey(int keySize) throws NoSuchAlgorithmException, IOException {
        KeyPairGenerator generator = KeyPairGenerator.getInstance("RSA");
        generator.initialize(keySize, new SecureRandom());
        KeyPair keyPair = generator.generateKeyPair();

        RSAPublicKey publicKey = (RSAPublicKey) keyPair.getPublic();
        RSAPrivateCrtKey privateKey = (RSAPrivateCrtKey) keyPair.getPrivate();

        return encodeOpenSSHPrivateKey(publicKey, privateKey);
    }

    public static String encodeOpenSSHPrivateKey(RSAPublicKey pub, RSAPrivateCrtKey priv) throws IOException {
        byte[] pubKeyBlob = createPublicKeyBlob(pub);
        byte[] privKeyBlob = createPrivateKeyBlob(pub, priv);

        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        baos.write("openssh-key-v1\0".getBytes(StandardCharsets.US_ASCII));

        writeString(baos, "none".getBytes(StandardCharsets.US_ASCII));
        writeString(baos, "none".getBytes(StandardCharsets.US_ASCII));
        writeString(baos, new byte[0]);
        writeInt(baos, 1);
        writeString(baos, pubKeyBlob);
        writeString(baos, privKeyBlob);

        byte[] fullKeyBytes = baos.toByteArray();
        String b64 = Base64.getEncoder().encodeToString(fullKeyBytes);

        StringBuilder sb = new StringBuilder();
        sb.append("-----BEGIN OPENSSH PRIVATE KEY-----\n");
        int index = 0;
        while (index < b64.length()) {
            int end = Math.min(index + 70, b64.length());
            sb.append(b64, index, end).append("\n");
            index = end;
        }
        sb.append("-----END OPENSSH PRIVATE KEY-----\n");

        return sb.toString();
    }

    private static byte[] createPublicKeyBlob(RSAPublicKey pub) throws IOException {
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        writeString(baos, "ssh-rsa".getBytes(StandardCharsets.US_ASCII));
        writeMpint(baos, pub.getPublicExponent());
        writeMpint(baos, pub.getModulus());
        return baos.toByteArray();
    }

    private static byte[] createPrivateKeyBlob(RSAPublicKey pub, RSAPrivateCrtKey priv) throws IOException {
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        int checkInt = new SecureRandom().nextInt();

        writeInt(baos, checkInt);
        writeInt(baos, checkInt);

        writeString(baos, "ssh-rsa".getBytes(StandardCharsets.US_ASCII));
        writeMpint(baos, pub.getModulus());
        writeMpint(baos, pub.getPublicExponent());
        writeMpint(baos, priv.getPrivateExponent());
        writeMpint(baos, priv.getCrtCoefficient());
        writeMpint(baos, priv.getPrimeP());
        writeMpint(baos, priv.getPrimeQ());
        writeString(baos, new byte[0]);

        int pad = 1;
        while (baos.size() % 8 != 0) {
            baos.write(pad++);
        }

        return baos.toByteArray();
    }

    private static void writeInt(ByteArrayOutputStream baos, int value) throws IOException {
        DataOutputStream dos = new DataOutputStream(baos);
        dos.writeInt(value);
        dos.flush();
    }

    private static void writeString(ByteArrayOutputStream baos, byte[] bytes) throws IOException {
        writeInt(baos, bytes.length);
        baos.write(bytes);
    }

    private static void writeMpint(ByteArrayOutputStream baos, BigInteger bi) throws IOException {
        if (bi.signum() == 0) {
            writeInt(baos, 0);
        } else {
            byte[] bytes = bi.toByteArray();
            writeString(baos, bytes);
        }
    }

    public static void main(String[] args) {
        try {
            int keySize = (args.length > 0) ? Integer.parseInt(args[0]) : 2048;
            String openSSHPrivateKey = generateOpenSSHPrivateKey(keySize);
            System.out.print(openSSHPrivateKey);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}