import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;
import java.security.spec.InvalidKeySpecException;
import java.security.spec.KeySpec;
import java.util.Base64;
import javax.crypto.SecretKeyFactory;
import javax.crypto.spec.PBEKeySpec;

public class PasswordHasher {

    private static final String ALGORITHM = "PBKDF2WithHmacSHA256";
    private static final int DEFAULT_ITERATIONS = 65536;
    private static final int KEY_LENGTH = 256;

    public static byte[] generateSalt(int length) {
        byte[] salt = new byte[length];
        SecureRandom random = new SecureRandom();
        random.nextBytes(salt);
        return salt;
    }

    public static byte[] hashPassword(char[] password, byte[] salt, int iterations, int keyLength)
            throws NoSuchAlgorithmException, InvalidKeySpecException {
        KeySpec spec = new PBEKeySpec(password, salt, iterations, keyLength);
        SecretKeyFactory factory = SecretKeyFactory.getInstance(ALGORITHM);
        return factory.generateSecret(spec).getEncoded();
    }

    public static String hashPasswordHex(char[] password, byte[] salt, int iterations, int keyLength)
            throws NoSuchAlgorithmException, InvalidKeySpecException {
        byte[] hash = hashPassword(password, salt, iterations, keyLength);
        StringBuilder hexString = new StringBuilder();
        for (byte b : hash) {
            hexString.append(String.format("%02x", b));
        }
        return hexString.toString();
    }

    public static String hashPasswordBase64(char[] password, byte[] salt)
            throws NoSuchAlgorithmException, InvalidKeySpecException {
        byte[] hash = hashPassword(password, salt, DEFAULT_ITERATIONS, KEY_LENGTH);
        return Base64.getEncoder().encodeToString(hash);
    }

    public static void main(String[] args) {
        try {
            String plainPassword = "SuperSecretPassword123!";
            byte[] salt = generateSalt(16);

            String saltBase64 = Base64.getEncoder().encodeToString(salt);
            String hexHash = hashPasswordHex(plainPassword.toCharArray(), salt, DEFAULT_ITERATIONS, KEY_LENGTH);
            String base64Hash = hashPasswordBase64(plainPassword.toCharArray(), salt);

            System.out.println("Salt (Base64): " + saltBase64);
            System.out.println("PBKDF2 Hash (Hex): " + hexHash);
            System.out.println("PBKDF2 Hash (Base64): " + base64Hash);
        } catch (NoSuchAlgorithmException | InvalidKeySpecException e) {
            e.printStackTrace();
        }
    }
}