import java.security.SecureRandom;
import java.util.Base64;

public class SessionIdGenerator {

    private static final SecureRandom SECURE_RANDOM = new SecureRandom();
    private static final Base64.Encoder URL_ENCODER = Base64.getUrlEncoder().withoutPadding();
    private static final int DEFAULT_BYTE_LENGTH = 32;

    public static String generateSessionId() {
        return generateSessionId(DEFAULT_BYTE_LENGTH);
    }

    public static String generateSessionId(int byteLength) {
        if (byteLength <= 0) {
            throw new IllegalArgumentException("Byte length must be greater than zero.");
        }
        byte[] randomBytes = new byte[byteLength];
        SECURE_RANDOM.nextBytes(randomBytes);
        return URL_ENCODER.encodeToString(randomBytes);
    }

    public static void main(String[] args) {
        String sessionId = generateSessionId();
        System.out.println("Generated Session ID: " + sessionId);
    }
}