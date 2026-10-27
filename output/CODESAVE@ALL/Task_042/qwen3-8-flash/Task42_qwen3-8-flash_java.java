import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;
import java.security.spec.InvalidKeySpecException;
import java.util.Base64;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import javax.crypto.SecretKeyFactory;
import javax.crypto.spec.PBEKeySpec;

public class Main {
    private static final int SALT_BYTES = 16;
    private static final int ITERATIONS = 210000;
    private static final int KEY_LENGTH = 256;
    private static final SecureRandom RANDOM = new SecureRandom();
    private static final Map<String, StoredCredential> USERS = new ConcurrentHashMap<>();

    private static final class StoredCredential {
        private final String username;
        private final String salt;
        private final String hash;

        StoredCredential(String username, String salt, String hash) {
            this.username = username;
            this.salt = salt;
            this.hash = hash;
        }
    }

    public static boolean registerUser(String username, String password) {
        if (!isValidInput(username, password)) {
            return false;
        }

        String normalizedUsername = normalizeUsername(username);
        String salt = generateSalt(SALT_BYTES);
        String hash = hashPassword(password, salt);

        return USERS.putIfAbsent(normalizedUsername, new StoredCredential(normalizedUsername, salt, hash)) == null;
    }

    public static boolean authenticateUser(String username, String password) {
        if (!isValidInput(username, password)) {
            return false;
        }

        StoredCredential credential = USERS.get(normalizeUsername(username));

        if (credential == null) {
            hashPassword(password, generateSalt(SALT_BYTES));
            return false;
        }

        String candidateHash = hashPassword(password, credential.salt);
        return constantTimeEquals(candidateHash, credential.hash);
    }