import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;
import java.security.spec.InvalidKeySpecException;
import java.security.spec.KeySpec;
import java.util.Arrays;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import javax.crypto.SecretKeyFactory;
import javax.crypto.spec.PBEKeySpec;

public class UserAuthenticationSystem {

    private static final String HASH_ALGORITHM = "PBKDF2WithHmacSHA256";
    private static final int ITERATION_COUNT = 120000;
    private static final int KEY_LENGTH_BITS = 256;
    private static final int SALT_LENGTH_BYTES = 16;

    private final SecureRandom secureRandom = new SecureRandom();
    private final Map<String, UserCredentials> credentialStore = new ConcurrentHashMap<>();

    private static final class UserCredentials {
        private final byte[] salt;
        private final byte[] passwordHash;

        UserCredentials(byte[] salt, byte[] passwordHash) {
            this.salt = salt.clone();
            this.passwordHash = passwordHash.clone();
        }

        byte[] getSalt() {
            return salt.clone();
        }

        byte[] getPasswordHash() {
            return passwordHash.clone();
        }
    }

    public boolean registerUser(String username, char[] password) {
        if (username == null || username.trim().isEmpty() || password == null || password.length == 0) {
            return false;
        }

        String normalizedUsername = username.trim().toLowerCase();
        if (credentialStore.containsKey(normalizedUsername)) {
            return false;
        }

        byte[] salt = new byte[SALT_LENGTH_BYTES];
        secureRandom.nextBytes(salt);

        byte[] hash = generateHash(password, salt);
        if (hash == null) {
            return false;
        }

        credentialStore.put(normalizedUsername, new UserCredentials(salt, hash));
        return true;
    }

    public boolean authenticate(String username, char[] password) {
        if (username == null || username.trim().isEmpty() || password == null || password.length == 0) {
            return false;
        }

        String normalizedUsername = username.trim().toLowerCase();
        UserCredentials storedCredentials = credentialStore.get(normalizedUsername);

        if (storedCredentials == null) {
            // Mitigate timing attacks by computing a dummy hash
            byte[] dummySalt = new byte[SALT_LENGTH_BYTES];
            generateHash(password, dummySalt);
            return false;
        }

        byte[] computedHash = generateHash(password, storedCredentials.getSalt());
        if (computedHash == null) {
            return false;
        }

        return MessageDigest.isEqual(computedHash, storedCredentials.getPasswordHash());
    }

    public boolean updatePassword(String username, char[] currentPassword, char[] newPassword) {
        if (username == null || currentPassword == null || newPassword == null || newPassword.length == 0) {
            return false;
        }

        if (!authenticate(username, currentPassword)) {
            return false;
        }

        String normalizedUsername = username.trim().toLowerCase();
        byte[] newSalt = new byte[SALT_LENGTH_BYTES];
        secureRandom.nextBytes(newSalt);

        byte[] newHash = generateHash(newPassword, newSalt);
        if (newHash == null) {
            return false;
        }

        credentialStore.put(normalizedUsername, new UserCredentials(newSalt, newHash));
        return true;
    }

    public boolean userExists(String username) {
        if (username == null) {
            return false;
        }
        return credentialStore.containsKey(username.trim().toLowerCase());
    }

    private byte[] generateHash(char[] password, byte[] salt) {
        KeySpec spec = new PBEKeySpec(password, salt, ITERATION_COUNT, KEY_LENGTH_BITS);
        try {
            SecretKeyFactory factory = SecretKeyFactory.getInstance(HASH_ALGORITHM);
            return factory.generateSecret(spec).getEncoded();
        } catch (NoSuchAlgorithmException | InvalidKeySpecException e) {
            return null;
        } finally {
            if (spec instanceof PBEKeySpec) {
                ((PBEKeySpec) spec).clearPassword();
            }
        }
    }

    public static void main(String[] args) {
        UserAuthenticationSystem authSystem = new UserAuthenticationSystem();

        char[] passAlice = "CorrectHorseBatteryStaple!1".toCharArray();
        boolean registered = authSystem.registerUser("Alice", passAlice);
        System.out.println("Registration status for Alice: " + registered);

        boolean authSuccess = authSystem.authenticate("alice", passAlice);
        System.out.println("Authentication with correct credentials: " + authSuccess);

        char[] wrongPass = "IncorrectPassword".toCharArray();
        boolean authFailure = authSystem.authenticate("alice", wrongPass);
        System.out.println("Authentication with incorrect credentials: " + authFailure);

        char[] newPassAlice = "Tr0ub4dor&3".toCharArray();
        boolean passwordUpdated = authSystem.updatePassword("Alice", passAlice, newPassAlice);
        System.out.println("Password update status: " + passwordUpdated);

        boolean oldPassAuth = authSystem.authenticate("Alice", passAlice);
        System.out.println("Authentication with old password: " + oldPassAuth);

        boolean newPassAuth = authSystem.authenticate("Alice", newPassAlice);
        System.out.println("Authentication with new password: " + newPassAuth);

        Arrays.fill(passAlice, '\0');
        Arrays.fill(wrongPass, '\0');
        Arrays.fill(newPassAlice, '\0');
    }
}