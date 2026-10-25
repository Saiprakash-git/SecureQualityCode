import java.security.GeneralSecurityException;
import java.security.MessageDigest;
import java.security.SecureRandom;
import java.security.spec.KeySpec;
import java.util.Arrays;
import java.util.Base64;
import java.util.HashMap;
import java.util.Map;
import java.util.regex.Pattern;
import javax.crypto.SecretKeyFactory;
import javax.crypto.spec.PBEKeySpec;

public class UserAuthSystem {

    public enum AuthResult {
        SUCCESS,
        USER_EXISTS,
        USER_NOT_FOUND,
        INVALID_USERNAME,
        WEAK_PASSWORD,
        INVALID_CREDENTIALS,
        LOCKED_OUT
    }

    private static final String KDF_ALGORITHM = "PBKDF2WithHmacSHA256";
    private static final int ITERATIONS = 310000;
    private static final int KEY_LENGTH_BITS = 256;
    private static final int SALT_LENGTH_BYTES = 16;
    private static final int MIN_PASSWORD_LENGTH = 12;
    private static final int MAX_PASSWORD_LENGTH = 128;
    private static final int MAX_FAILED_ATTEMPTS = 5;
    private static final long LOCKOUT_MILLIS = 15L * 60L * 1000L;
    private static final Pattern USERNAME_PATTERN = Pattern.compile("^[A-Za-z0-9_]{3,32}$");

    private static final class UserRecord {
        final String username;
        final byte[] salt;
        final byte[] hash;
        int failedAttempts;
        long lockedUntil;

        UserRecord(String username, byte[] salt, byte[] hash) {
            this.username = username;
            this.salt = salt;
            this.hash = hash;
            this.failedAttempts = 0;
            this.lockedUntil = 0L;
        }
    }

    private final Map<String, UserRecord> users = new HashMap<>();
    private final SecureRandom random = new SecureRandom();
    private final byte[] dummySalt;
    private final byte[] dummyHash;

    public UserAuthSystem() {
        this.dummySalt = randomSalt();
        this.dummyHash = deriveKey(new char[] {'d', 'u', 'm', 'm', 'y'}, dummySalt, ITERATIONS);
    }

    public synchronized AuthResult register(String username, char[] password) {
        try {
            if (!isValidUsername(username)) {
                return AuthResult.INVALID_USERNAME;
            }
            if (!isStrongPassword(password)) {
                return AuthResult.WEAK_PASSWORD;
            }
            String key = normalize(username);
            if (users.containsKey(key)) {
                return AuthResult.USER_EXISTS;
            }
            byte[] salt = randomSalt();
            byte[] hash = deriveKey(password, salt, ITERATIONS);
            users.put(key, new UserRecord(username, salt, hash));
            return AuthResult.SUCCESS;
        } finally {
            wipe(password);
        }
    }

    public synchronized AuthResult login(String username, char[] password) {
        try {
            String key = normalize(username);
            UserRecord record = key == null ? null : users.get(key);
            long now = System.currentTimeMillis();

            if (record == null) {
                // Perform equivalent work to reduce user-enumeration timing differences.
                deriveKey(password == null ? new char[0] : password, dummySalt, ITERATIONS);
                return AuthResult.INVALID_CREDENTIALS;
            }
            if (record.lockedUntil > now) {
                return AuthResult.LOCKED_OUT;
            }

            byte[] candidate = deriveKey(password == null ? new char[0] : password, record.salt, ITERATIONS);
            boolean matches = constantTimeEquals(candidate, record.hash);

            if (matches) {
                record.failedAttempts = 0;
                record.lockedUntil = 0L;
                return AuthResult.SUCCESS;
            }

            record.failedAttempts++;
            if (record.failedAttempts >= MAX_FAILED_ATTEMPTS) {
                record.lockedUntil = now + LOCKOUT_MILLIS;
                record.failedAttempts = 0;
            }
            return AuthResult.INVALID_CREDENTIALS;
        } finally {
            wipe(password);
        }
    }

    public synchronized AuthResult changePassword(String username, char[] oldPassword, char[] newPassword) {
        try {
            AuthResult loginResult = login(username, oldPassword);
            if (loginResult != AuthResult.SUCCESS) {
                return loginResult;
            }
            if (!isStrongPassword(newPassword)) {
                return AuthResult.WEAK_PASSWORD;
            }
            UserRecord record = users.get(normalize(username));
            if (record == null) {
                return AuthResult.USER_NOT_FOUND;
            }
            byte[] salt = randomSalt();
            byte[] hash = deriveKey(newPassword, salt, ITERATIONS);
            users.put(normalize(username), new UserRecord(record.username, salt, hash));
            return AuthResult.SUCCESS;
        } finally {
            wipe(oldPassword);
            wipe(newPassword);
        }
    }

    public synchronized boolean userExists(String username) {
        String key = normalize(username);
        return key != null && users.containsKey(key);
    }

    public synchronized String exportCredential(String username) {
        UserRecord record = users.get(normalize(username));
        if (record == null) {
            return null;
        }
        return ITERATIONS + ":" + Base64.getEncoder().encodeToString(record.salt)
                + ":" + Base64.getEncoder().encodeToString(record.hash);
    }

    public static boolean isValidUsername(String username) {
        return username != null && USERNAME_PATTERN.matcher(username).matches();
    }

    public static boolean isStrongPassword(char[] password) {
        if (password == null || password.length < MIN_PASSWORD_LENGTH || password.length > MAX_PASSWORD_LENGTH) {
            return false;
        }
        boolean upper = false, lower = false, digit = false, symbol = false;
        for (char c : password) {
            if (Character.isUpperCase(c)) upper = true;
            else if (Character.isLowerCase(c)) lower = true;
            else if (Character.isDigit(c)) digit = true;
            else if (!Character.isWhitespace(c)) symbol = true;
        }
        return upper && lower && digit && symbol;
    }

    private static String normalize(String username) {
        return username == null ? null : username.trim().toLowerCase();
    }

    private byte[] randomSalt() {
        byte[] salt = new byte[SALT_LENGTH_BYTES];
        random.nextBytes(salt);
        return salt;
    }

    private static byte[] deriveKey(char[] password, byte[] salt, int iterations) {
        PBEKeySpec spec = new PBEKeySpec(password, salt, iterations, KEY_LENGTH_BITS);
        try {
            SecretKeyFactory factory = SecretKeyFactory.getInstance(KDF_ALGORITHM);
            KeySpec ks = spec;
            return factory.generateSecret(ks).getEncoded();
        } catch (GeneralSecurityException e) {
            throw new IllegalStateException("Password hashing failed", e);
        } finally {
            spec.clearPassword();
        }
    }

    private static boolean constantTimeEquals(byte[] a, byte[] b) {
        return MessageDigest.isEqual(a, b);
    }

    private static void wipe(char[] data) {
        if (data != null) {
            Arrays.fill(data, '\0');
        }
    }

    public static void main(String[] args) {
        UserAuthSystem auth = new UserAuthSystem();

        char[] pw1 = "Str0ng!Passw0rd".toCharArray();
        System.out.println("Register: " + auth.register("alice_01", pw1));

        System.out.println("Login wrong: " + auth.login("alice_01", "bad".toCharArray()));
        System.out.println("Login right: " + auth.login("alice_01", "Str0ng!Passw0rd".toCharArray()));

        System.out.println("Change password: " +