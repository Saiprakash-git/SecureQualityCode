import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;
import java.security.spec.InvalidKeySpecException;
import java.util.Base64;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import javax.crypto.SecretKeyFactory;
import javax.crypto.spec.PBEKeySpec;

public class UserAuthSystem {
    private static final int ITERATIONS = 100000;
    private static final int KEY_LENGTH = 256;
    private static final int SALT_LENGTH = 16;
    private static final String ALGORITHM = "PBKDF2WithHmacSHA256";
    private static final int MIN_PASSWORD_LENGTH = 8;

    private final Map<String, UserRecord> users;
    private final SecureRandom secureRandom;

    public UserAuthSystem() {
        this.users = new ConcurrentHashMap<>();
        this.secureRandom = new SecureRandom();
    }

    private static final class UserRecord {
        private final String salt;
        private final String hash;

        private UserRecord(String salt, String hash) {
            this.salt = salt;
            this.hash = hash;
        }
    }

    public boolean register(String username, String password) {
        if (!isValidInput(username, password)) {
            return false;
        }

        if (users.containsKey(username)) {
            return false;
        }

        byte[] salt = generateSalt();
        String saltEncoded = Base64.getEncoder().encodeToString(salt);
        String hash = hashPassword(password, salt);

        UserRecord record = new UserRecord(saltEncoded, hash);
        return users.putIfAbsent(username, record) == null;
    }

    public boolean authenticate(String username, String password) {
        if (username == null || password == null) {
            return false;
        }

        UserRecord record = users.get(username);
        if (record == null) {
            return false;
        }

        byte[] salt = Base64.getDecoder().decode(record.salt);
        String computedHash = hashPassword(password, salt);

        return MessageDigest.isEqual(
            computedHash.getBytes(),
            record.hash.getBytes()
        );
    }

    public boolean changePassword(String username, String currentPassword, String newPassword) {
        if (!authenticate(username, currentPassword)) {
            return false;
        }

        if (newPassword == null || newPassword.length() < MIN_PASSWORD_LENGTH) {
            return false;
        }

        byte[] salt = generateSalt();
        String saltEncoded = Base64.getEncoder().encodeToString(salt);
        String hash = hashPassword(newPassword, salt);

        users.put(username, new UserRecord(saltEncoded, hash));
        return true;
    }

    public boolean deleteUser(String username, String password) {
        if (!authenticate(username, password)) {
            return false;
        }

        return users.remove(username) != null;
    }

    public boolean userExists(String username) {
        return username != null && users.containsKey(username);
    }

    private boolean isValidInput(String username, String password) {
        return username != null
            && !username.trim().isEmpty()
            && password != null
            && password.length() >= MIN_PASSWORD_LENGTH;
    }

    private String hashPassword(String password, byte[] salt) {
        try {
            PBEKeySpec spec = new PBEKeySpec(
                password.toCharArray(),
                salt,
                ITERATIONS,
                KEY_LENGTH
            );
            SecretKeyFactory factory = SecretKeyFactory.getInstance(ALGORITHM);
            byte[] hash = factory.generateSecret(spec).getEncoded();
            return Base64.getEncoder().encodeToString(hash);
        } catch (NoSuchAlgorithmException | InvalidKeySpecException e) {
            throw new SecurityException("Failed to hash password", e);
        }
    }

    private byte[] generateSalt() {
        byte[] salt = new byte[SALT_LENGTH];
        secureRandom.nextBytes(salt);
        return salt;
    }
}