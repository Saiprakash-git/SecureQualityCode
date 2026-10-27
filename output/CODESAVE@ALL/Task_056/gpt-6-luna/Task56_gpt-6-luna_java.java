import java.security.SecureRandom;
import java.util.Base64;

public final class AuthenticationTokenGenerator {
    private static final SecureRandom SECURE_RANDOM = new SecureRandom();

    private AuthenticationTokenGenerator() {
    }

    public static String generateToken(int entropyBytes) {
        if (entropyBytes < 16 || entropyBytes > 1024) {
            throw new IllegalArgumentException("Entropy must be between 16 and 1024 bytes.");
        }

        byte[] tokenBytes = new byte[entropyBytes];
        SECURE_RANDOM.nextBytes(tokenBytes);
        return Base64.getUrlEncoder().withoutPadding().encodeToString(tokenBytes);
    }

    public static void main(String[] args) {
        int entropyBytes = args.length == 0 ? 32 : Integer.parseInt(args[0]);
        System.out.println(generateToken(entropyBytes));
    }
}