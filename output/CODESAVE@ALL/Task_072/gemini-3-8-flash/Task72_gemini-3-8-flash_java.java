import javax.crypto.Cipher;
import javax.crypto.spec.IvParameterSpec;
import javax.crypto.spec.SecretKeySpec;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;

public class AesCbcEncryptor {

    // Step 1: Create a static initialization vector (IV)
    private static final byte[] STATIC_IV = new byte[] {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
    };

    /**
     * Pads the input data using PKCS#7 padding to align with the cipher's block size.
     *
     * @param data      The unpadded input data.
     * @param blockSize The block size in bytes.
     * @return Padded byte array.
     */
    public static byte[] padData(byte[] data, int blockSize) {
        int paddingLength = blockSize - (data.length % blockSize);
        byte[] padded = new byte[data.length + paddingLength];
        System.arraycopy(data, 0, padded, 0, data.length);
        Arrays.fill(padded, data.length, padded.length, (byte) paddingLength);
        return padded;
    }

    /**
     * Encrypts the input data using AES in CBC mode with a static IV.
     *
     * @param data     The plaintext bytes to encrypt.
     * @param keyBytes The encryption key (16, 24, or 32 bytes).
     * @return The encrypted ciphertext bytes.
     * @throws Exception If an encryption error occurs.
     */
    public static byte[] encrypt(byte[] data, byte[] keyBytes) throws Exception {
        // Step 1: Wrap static IV
        IvParameterSpec ivSpec = new IvParameterSpec(STATIC_IV);
        SecretKeySpec keySpec = new SecretKeySpec(keyBytes, "AES");

        // Step 2: Initialize a cipher using AES in CBC mode with the static IV
        Cipher cipher = Cipher.getInstance("AES/CBC/NoPadding");
        cipher.init(Cipher.ENCRYPT_MODE, keySpec, ivSpec);

        // Step 3: Pad the input data to match the block size required by the cipher
        byte[] paddedData = padData(data, cipher.getBlockSize());

        // Step 4: Encrypt the padded data using the cipher
        byte[] encryptedData = cipher.doFinal(paddedData);

        // Step 5: Return the encrypted data
        return encryptedData;
    }

    public static void main(String[] args) throws Exception {
        byte[] key = new byte[16]; // 128-bit key
        byte[] plaintext = "Hello, World! This is a test message.".getBytes(StandardCharsets.UTF_8);

        byte[] ciphertext = encrypt(plaintext, key);

        System.out.println("Plaintext length: " + plaintext.length);
        System.out.println("Ciphertext length: " + ciphertext.length);
    }
}