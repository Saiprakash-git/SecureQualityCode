import java.security.GeneralSecurityException;
import java.security.MessageDigest;
import java.security.SecureRandom;
import java.util.Arrays;
import java.util.Locale;
import java.util.concurrent.ConcurrentHashMap;
import javax.crypto.SecretKeyFactory;
import javax.crypto.spec.PBEKeySpec;

public final class UserAuthenticationSystem {
    private static final int SALT_LENGTH_BYTES = 16;
    private static final int HASH_LENGTH_BYTES = 32;
    private static final int PBKDF2_ITERATIONS = 310_000;
    private static final int MIN_PASSWORD_LENGTH = 12;
    private static final int MAX_PASSWORD_LENGTH = 1024;

    private final SecureRandom secureRandom = new SecureRandom();
    private final ConcurrentHashMap<String, Credential> users = new ConcurrentHashMap<>();
    private final byte[] dummySalt = new byte[SALT_LENGTH_BYTES];
    private final byte[] dummyHash = new byte[HASH_LENGTH_BYTES];

    public UserAuthenticationSystem() {
        secureRandom.nextBytes(dummySalt);
        secureRandom.nextBytes(dummyHash);
    }

    public boolean register(String username, char[] password) {
        String normalizedUsername = normalizeUsername(username);
        if (normalizedUsername == null || !isValidNewPassword(password)) {
            return false;
        }

        byte[] salt = new byte[SALT_LENGTH_BYTES];
        secureRandom.nextBytes(salt);
        byte[] hash = deriveHash(password, salt);
        Credential credential = new Credential(salt, hash);

        return users.putIfAbsent(normalizedUsername, credential) == null;
    }

    public boolean authenticate(String username, char[] password) {
        String normalizedUsername = normalizeUsername(username);
        if (password == null || password.length > MAX_PASSWORD_LENGTH) {
            return false;
        }

        Credential credential = normalizedUsername == null
                ? null
                : users.get(normalizedUsername);

        byte[] salt = credential == null ? dummySalt : credential.salt;
        byte[] expectedHash = credential == null ? dummyHash : credential.hash;
        byte[] actualHash = deriveHash(password, salt);
        boolean matches = MessageDigest.isEqual(actualHash, expectedHash);

        return credential != null && matches;
    }

    private static boolean isValidNewPassword(char[] password) {
        return password != null
                && password.length >= MIN_PASSWORD_LENGTH
                && password.length <= MAX_PASSWORD_LENGTH;
    }

    private static String normalizeUsername(String username) {
        if (username == null || username.length() < 3 || username.length() > 64) {
            return null;
        }

        for (int i = 0; i < username.length(); i++) {
            char c = username.charAt(i);
            boolean allowed = (c >= 'a' && c <= 'z')
                    || (c >= 'A' && c <= 'Z')
                    || (c >= '0' && c <= '9')
                    || c == '_'
                    || c == '-'
                    || c == '.';
            if (!allowed) {
                return null;
            }
        }

        return username.toLowerCase(Locale.ROOT);
    }

    private static byte[] deriveHash(char[] password, byte[] salt) {
        char[] passwordCopy = password.clone();
        PBEKeySpec keySpec = new PBEKeySpec(
                passwordCopy,
                salt,
                PBKDF2_ITERATIONS,
                HASH_LENGTH_BYTES * 8
        );

        try {
            SecretKeyFactory factory = SecretKeyFactory.getInstance("PBKDF2WithHmacSHA256");
            return factory.generateSecret(keySpec).getEncoded();
        } catch (GeneralSecurityException exception) {
            throw new IllegalStateException("Unable to derive password hash", exception);
        } finally {
            keySpec.clearPassword();
            Arrays.fill(passwordCopy, '\0');
        }
    }

    private static final class Credential {
        private final byte[] salt;
        private final byte[] hash;

        private Credential(byte[] salt, byte[] hash) {
            this.salt = salt.clone();
            this.hash = hash.clone();
        }
    }
}