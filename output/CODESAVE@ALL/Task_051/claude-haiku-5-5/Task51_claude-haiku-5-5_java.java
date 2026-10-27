import java.nio.charset.StandardCharsets;
import java.security.SecureRandom;
import java.util.Base64;
import javax.crypto.Cipher;
import javax.crypto.SecretKey;
import javax.crypto.SecretKeyFactory;
import javax.crypto.spec.GCMParameterSpec;
import javax.crypto.spec.PBEKeySpec;
import javax.crypto.spec.SecretKeySpec;

public class Main {

    private static final int SALT_LENGTH = 16;
    private static final int IV_LENGTH = 12;
    private static final int KEY_LENGTH_BITS = 256;
    private static final int GCM_TAG_LENGTH_BITS = 128;
    private static final int PBKDF2_ITERATIONS = 65536;

    public static String encrypt(String plaintext, String password) throws Exception {
        if (plaintext == null) {
            throw new IllegalArgumentException("Plaintext must not be null");
        }
        if (password == null || password.isEmpty()) {
            throw new IllegalArgumentException("Password must not be empty");
        }

        SecureRandom random = new SecureRandom();

        byte[] salt = new byte[SALT_LENGTH];
        random.nextBytes(salt);

        byte[] iv = new byte[IV_LENGTH];
        random.nextBytes(iv);

        SecretKey key = deriveKey(password, salt);

        Cipher cipher = Cipher.getInstance("AES/GCM/NoPadding");
        cipher.init(Cipher.ENCRYPT_MODE, key, new GCMParameterSpec(GCM_TAG_LENGTH_BITS, iv));
        byte[] ciphertext = cipher.doFinal(plaintext.getBytes(StandardCharsets.UTF_8));

        byte[] output = new byte[salt.length + iv.length + ciphertext.length];
        System.arraycopy(salt, 0, output, 0, salt.length);
        System.arraycopy(iv, 0, output, salt.length, iv.length);
        System.arraycopy(ciphertext, 0, output, salt.length + iv.length, ciphertext.length);

        return Base64.getEncoder().encodeToString(output);
    }

    private static SecretKey deriveKey(String password, byte[] salt) throws Exception {
        PBEKeySpec spec = new PBEKeySpec(password.toCharArray(), salt, PBKDF2_ITERATIONS, KEY_LENGTH_BITS);
        try {
            SecretKeyFactory factory = SecretKeyFactory.getInstance("PBKDF2WithHmacSHA256");
            byte[] keyBytes = factory.generateSecret(spec).getEncoded();
            return new SecretKeySpec(keyBytes, "AES");
        } finally {
            spec.clearPassword();
        }
    }

    public static void main(String[] args) {
        String plaintext = args.length > 0 ? args[0] : "Hello, World!";
        String password = args.length > 1 ? args[1] : "examplePassword";

        try {
            String encrypted = encrypt(plaintext, password);
            System.out.println(encrypted);
        } catch (Exception e) {
            System.err.println("Encryption failed: " + e.getMessage());
            System.exit(1);
        }
    }
}