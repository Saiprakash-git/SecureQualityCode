import javax.crypto.Cipher;
import javax.crypto.SecretKey;
import javax.crypto.spec.IvParameterSpec;
import javax.crypto.spec.SecretKeySpec;
import java.nio.charset.StandardCharsets;
import java.security.SecureRandom;
import java.util.Arrays;
import java.util.Base64;

public class AesCbcStaticIvEncryptor {

    private static final int BLOCK_SIZE = 16;

    private static final byte[] STATIC_IV = new byte[] {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
    };

    public static byte[] encrypt(byte[] data, byte[] keyBytes) throws Exception {
        if (data == null) {
            throw new IllegalArgumentException("Input data must not be null");
        }
        if (keyBytes == null || (keyBytes.length != 16 && keyBytes.length != 24 && keyBytes.length != 32)) {
            throw new IllegalArgumentException("Key must be 16, 24, or 32 bytes");
        }

        SecretKey key = new SecretKeySpec(keyBytes, "AES");
        Cipher cipher = Cipher.getInstance("AES/CBC/NoPadding");
        cipher.init(Cipher.ENCRYPT_MODE, key, new IvParameterSpec(STATIC_IV));

        byte[] padded = pkcs7Pad(data, BLOCK_SIZE);
        return cipher.doFinal(padded);
    }

    private static byte[] pkcs7Pad(byte[] data, int blockSize) {
        int padLength = blockSize - (data.length % blockSize);
        byte[] result = Arrays.copyOf(data, data.length + padLength);
        for (int i = data.length; i < result.length; i++) {
            result[i] = (byte) padLength;
        }
        return result;
    }

    public static String encryptToBase64(String plaintext, byte[] keyBytes) throws Exception {
        byte[] encrypted = encrypt(plaintext.getBytes(StandardCharsets.UTF_8), keyBytes);
        return Base64.getEncoder().encodeToString(encrypted);
    }

    public static void main(String[] args) throws Exception {
        byte[] key = new byte[16];
        new SecureRandom().nextBytes(key);

        String message = args.length > 0 ? args[0] : "Hello, AES CBC with static IV!";
        byte[] encrypted = encrypt(message.getBytes(StandardCharsets.UTF_8), key);
        System.out.println(Base64.getEncoder().encodeToString(encrypted));
    }
}