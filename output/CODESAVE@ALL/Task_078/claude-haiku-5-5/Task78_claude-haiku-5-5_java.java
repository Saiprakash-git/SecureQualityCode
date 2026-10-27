import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.math.BigInteger;
import java.nio.charset.StandardCharsets;
import java.security.GeneralSecurityException;
import java.security.KeyPair;
import java.security.KeyPairGenerator;
import java.security.SecureRandom;
import java.security.interfaces.RSAPrivateCrtKey;
import java.security.interfaces.RSAPublicKey;
import java.util.Base64;

public class OpenSSHRsaKeyGenerator {

    private static final String AUTH_MAGIC = "openssh-key-v1";
    private static final String KEY_TYPE = "ssh-rsa";
    private static final int CIPHER_BLOCK_SIZE = 8;

    public static String generateOpenSSHPrivateKey(int bits, String comment)
            throws GeneralSecurityException, IOException {
        if (bits < 1024) {
            throw new IllegalArgumentException("RSA key size must be at least 1024 bits");
        }
        if (comment == null) {
            comment = "";
        }

        KeyPairGenerator kpg = KeyPairGenerator.getInstance("RSA");
        kpg.initialize(bits, new SecureRandom());
        KeyPair kp = kpg.generateKeyPair();

        RSAPublicKey pub = (RSAPublicKey) kp.getPublic();
        RSAPrivateCrtKey priv = (RSAPrivateCrtKey) kp.getPrivate();

        // Public key blob: string "ssh-rsa", mpint e, mpint n
        ByteArrayOutputStream pubBlobBytes = new ByteArrayOutputStream();
        DataOutputStream pubBlob = new DataOutputStream(pubBlobBytes);
        writeString(pubBlob, KEY_TYPE.getBytes(StandardCharsets.US_ASCII));
        writeMpint(pubBlob, pub.getPublicExponent());
        writeMpint(pubBlob, pub.getModulus());
        pubBlob.flush();
        byte[] publicBlob = pubBlobBytes.toByteArray();

        // Private section (unencrypted)
        SecureRandom rnd = new SecureRandom();
        int checkInt = rnd.nextInt();

        ByteArrayOutputStream privBytes = new ByteArrayOutputStream();
        DataOutputStream priv0 = new DataOutputStream(privBytes);
        priv0.writeInt(checkInt);
        priv0.writeInt(checkInt);
        writeString(priv0, KEY_TYPE.getBytes(StandardCharsets.US_ASCII));
        writeMpint(priv0, priv.getModulus());
        writeMpint(priv0, priv.getPublicExponent());
        writeMpint(priv0, priv.getPrivateExponent());
        writeMpint(priv0, priv.getCrtCoefficient());
        writeMpint(priv0, priv.getPrimeP());
        writeMpint(priv0, priv.getPrimeQ());
        writeString(priv0, comment.getBytes(StandardCharsets.UTF_8));

        int padLen = (CIPHER_BLOCK_SIZE - (privBytes.size() % CIPHER_BLOCK_SIZE)) % CIPHER_BLOCK_SIZE;
        for (int i = 1; i <= padLen; i++) {
            priv0.writeByte(i);
        }
        priv0.flush();
        byte[] privateSection = privBytes.toByteArray();

        // Full binary structure
        ByteArrayOutputStream outBytes = new ByteArrayOutputStream();
        DataOutputStream out = new DataOutputStream(outBytes);
        out.write(AUTH_MAGIC.getBytes(StandardCharsets.US_ASCII));
        out.writeByte(0);
        writeString(out, "none".getBytes(StandardCharsets.US_ASCII)); // cipher
        writeString(out, "none".getBytes(StandardCharsets.US_ASCII)); // kdf
        writeString(out, new byte[0]);                                 // kdf options
        out.writeInt(1);                                               // number of keys
        writeString(out, publicBlob);
        writeString(out, privateSection);
        out.flush();

        String b64 = Base64.getEncoder().encodeToString(outBytes.toByteArray());

        StringBuilder sb = new StringBuilder();
        sb.append("-----BEGIN OPENSSH PRIVATE KEY-----\n");
        for (int i = 0; i < b64.length(); i += 70) {
            sb.append(b64, i, Math.min(i + 70, b64.length())).append('\n');
        }
        sb.append("-----END OPENSSH PRIVATE KEY-----\n");
        return sb.toString();
    }

    private static void writeString(DataOutputStream out, byte[] data) throws IOException {
        out.writeInt(data.length);
        out.write(data);
    }

    private static void writeMpint(DataOutputStream out, BigInteger value) throws IOException {
        writeString(out, value.toByteArray());
    }

    public static void main(String[] args) throws Exception {
        int bits = 3072;
        String comment = "generated-rsa-key";
        if (args.length > 0) {
            bits = Integer.parseInt(args[0]);
        }
        if (args.length > 1) {
            comment = args[1];
        }
        System.out.print(generateOpenSSHPrivateKey(bits, comment));
    }
}