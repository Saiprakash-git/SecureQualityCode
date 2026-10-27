import javax.crypto.Cipher;
import javax.crypto.KeyGenerator;
import javax.crypto.SecretKey;
import javax.crypto.spec.GCMParameterSpec;
import javax.crypto.spec.SecretKeySpec;
import java.nio.charset.StandardCharsets;
import java.security.SecureRandom;
import java.util.Base64;

public class SecureDataStore {
    private static final String ALGORITHM = "AES/GCM/NoPadding";
    private static final int TAG_LENGTH_BIT = 128;
    private static final int IV_LENGTH_BYTE = 12;
    private final SecretKey secretKey;

    public SecureDataStore() throws Exception {
        KeyGenerator keyGen = KeyGenerator.getInstance("AES");
        keyGen.init(256);
        this.secretKey = keyGen.generateKey();
    }

    public SecureDataStore(String base64Key) {
        byte[] keyBytes = Base64.getDecoder().decode(base64Key);
        this.secretKey = new SecretKeySpec(keyBytes, "AES");
    }

    public String encrypt(String sensitiveData) throws Exception {
        byte[] iv = new byte[IV_LENGTH_BYTE];
        SecureRandom random = new SecureRandom();
        random.nextBytes(iv);

        Cipher cipher = Cipher.getInstance(ALGORITHM);
        GCMParameterSpec parameterSpec = new GCMParameterSpec(TAG_LENGTH_BIT, iv);
        cipher.init(Cipher.ENCRYPT_MODE, secretKey, parameterSpec);

        byte[] cipherText = cipher.doFinal(sensitiveData.getBytes(StandardCharsets.UTF_8));

        byte[] encrypted = new byte[iv.length + cipherText.length];
        System.arraycopy(iv, 0, encrypted, 0, iv.length);
        System.arraycopy(cipherText, 0, encrypted, iv.length, cipherText.length);

        return Base64.getEncoder().encodeToString(encrypted);
    }

    public String decrypt(String encryptedData) throws Exception {
        byte[] decoded = Base64.getDecoder().decode(encryptedData);

        byte[] iv = new byte[IV_LENGTH_BYTE];
        System.arraycopy(decoded, 0, iv, 0, iv.length);

        byte[] cipherText = new byte[decoded.length - IV_LENGTH_BYTE];
        System.arraycopy(decoded, IV_LENGTH_BYTE, cipherText, 0, cipherText.length);

        Cipher cipher = Cipher.getInstance(ALGORITHM);
        GCMParameterSpec parameterSpec = new GCMParameterSpec(TAG_LENGTH_BIT, iv);
        cipher.init(Cipher.DECRYPT_MODE, secretKey, parameterSpec);

        byte[] decrypted = cipher.doFinal(cipherText);
        return new String(decrypted, StandardCharsets.UTF_8);
    }

    public String getEncodedKey() {
        return Base64.getEncoder().encodeToString(secretKey.getEncoded());
    }

    public static void main(String[] args) throws Exception {
        SecureDataStore store = new SecureDataStore();
        String sensitiveData = "4111-1111-1111-1111";
        String encrypted = store.encrypt(sensitiveData);
        String decrypted = store.decrypt(encrypted);
        System.out.println(encrypted);
        System.out.println(decrypted);
    }
}