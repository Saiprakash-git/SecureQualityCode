import javax.crypto.Cipher;
import javax.crypto.KeyGenerator;
import javax.crypto.SecretKey;
import javax.crypto.spec.GCMParameterSpec;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;
import java.util.Base64;

public class StringEncryptor {

    private static final String ENCRYPTION_ALGORITHM = "AES/GCM/NoPadding";
    private static final int TAG_LENGTH_BIT = 128;
    private static final int IV_LENGTH_BYTE = 12;
    private static final int KEY_SIZE_BIT = 256;

    public static SecretKey generateKey() throws NoSuchAlgorithmException {
        KeyGenerator keyGenerator = KeyGenerator.getInstance("AES");
        keyGenerator.init(KEY_SIZE_BIT);
        return keyGenerator.generateKey();
    }

    public static String encrypt(String plainText, SecretKey key) throws Exception {
        byte[] iv = new byte[IV_LENGTH_BYTE];
        SecureRandom random = new SecureRandom();
        random.nextBytes(iv);

        Cipher cipher = Cipher.getInstance(ENCRYPTION_ALGORITHM);
        GCMParameterSpec parameterSpec = new GCMParameterSpec(TAG_LENGTH_BIT, iv);
        cipher.init(Cipher.ENCRYPT_MODE, key, parameterSpec);

        byte[] cipherText = cipher.doFinal(plainText.getBytes(StandardCharsets.UTF_8));

        ByteBuffer byteBuffer = ByteBuffer.allocate(iv.length + cipherText.length);
        byteBuffer.put(iv);
        byteBuffer.put(cipherText);

        return Base64.getEncoder().encodeToString(byteBuffer.array());
    }

    public static String decrypt(String encryptedTextBase64, SecretKey key) throws Exception {
        byte[] cipherTextWithIv = Base64.getDecoder().decode(encryptedTextBase64);

        ByteBuffer byteBuffer = ByteBuffer.wrap(cipherTextWithIv);
        byte[] iv = new byte[IV_LENGTH_BYTE];
        byteBuffer.get(iv);

        byte[] cipherText = new byte[byteBuffer.remaining()];
        byteBuffer.get(cipherText);

        Cipher cipher = Cipher.getInstance(ENCRYPTION_ALGORITHM);
        GCMParameterSpec parameterSpec = new GCMParameterSpec(TAG_LENGTH_BIT, iv);
        cipher.init(Cipher.DECRYPT_MODE, key, parameterSpec);

        byte[] plainText = cipher.doFinal(cipherText);
        return new String(plainText, StandardCharsets.UTF_8);
    }

    public static void main(String[] args) {
        try {
            String originalString = "Confidential Data Payload";
            SecretKey secretKey = generateKey();

            String encryptedString = encrypt(originalString, secretKey);
            String decryptedString = decrypt(encryptedString, secretKey);

            System.out.println("Original:  " + originalString);
            System.out.println("Encrypted: " + encryptedString);
            System.out.println("Decrypted: " + decryptedString);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}