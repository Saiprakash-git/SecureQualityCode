import java.nio.charset.StandardCharsets;
import java.security.GeneralSecurityException;
import javax.crypto.SecretKeyFactory;
import javax.crypto.spec.PBEKeySpec;

public final class PasswordHasher {
    private static final int ITERATIONS = 600_000;
    private static final int KEY_LENGTH_BITS = 256;

    private PasswordHasher() {
    }

    public static String hashPassword(String password, String salt) {
        if (password == null || salt == null) {
            throw new IllegalArgumentException("Password and salt must not be null.");
        }

        PBEKeySpec spec = new PBEKeySpec(
                password.toCharArray(),
                salt.getBytes(StandardCharsets.UTF_8),
                ITERATIONS,
                KEY_LENGTH_BITS
        );

        try {
            byte[] hash = SecretKeyFactory.getInstance("PBKDF2WithHmacSHA256")
                    .generateSecret(spec)
                    .getEncoded();
            return toHex(hash);
        } catch (GeneralSecurityException e) {
            throw new IllegalStateException("Unable to hash password.", e);
        } finally {
            spec.clearPassword();
        }
    }

    private static String toHex(byte[] bytes) {
        StringBuilder result = new StringBuilder(bytes.length * 2);
        for (byte value : bytes) {
            result.append(Character.forDigit((value >>> 4) & 0x0F, 16));
            result.append(Character.forDigit(value & 0x0F, 16));
        }
        return result.toString();
    }
}