import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;
import java.util.Base64;

public class PasswordHasher {

    public static String generateSalt(int length) {
        byte[] salt = new byte[length];
        new SecureRandom().nextBytes(salt);
        return Base64.getEncoder().encodeToString(salt);
    }

    public static String hashPassword(String password, String salt, String algorithm) throws NoSuchAlgorithmException {
        MessageDigest digest = MessageDigest.getInstance(algorithm);
        digest.update(Base64.getDecoder().decode(salt));
        byte[] hash = digest.digest(password.getBytes(StandardCharsets.UTF_8));
        return Base64.getEncoder().encodeToString(hash);
    }

    public static boolean verifyPassword(String password, String salt, String expectedHash, String algorithm) throws NoSuchAlgorithmException {
        String actualHash = hashPassword(password, salt, algorithm);
        return MessageDigest.isEqual(
                actualHash.getBytes(StandardCharsets.UTF_8),
                expectedHash.getBytes(StandardCharsets.UTF_8));
    }

    public static void main(String[] args) throws NoSuchAlgorithmException {
        String password = args.length > 0 ? args[0] : "password123";
        String salt = generateSalt(16);
        String hash = hashPassword(password, salt, "SHA-256");
        System.out.println("Salt: " + salt);
        System.out.println("Hash: " + hash);
        System.out.println("Verified: " + verifyPassword(password, salt, hash, "SHA-256"));
    }
}