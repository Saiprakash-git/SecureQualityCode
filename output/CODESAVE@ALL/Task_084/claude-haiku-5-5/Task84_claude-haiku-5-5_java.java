import java.security.SecureRandom;
import java.util.Base64;
import java.util.UUID;

public class SessionIdGenerator {

    private static final SecureRandom SECURE_RANDOM = new SecureRandom();

    public static String generateSessionId() {
        byte[] randomBytes = new byte[32];
        SECURE_RANDOM.nextBytes(randomBytes);
        String random = Base64.getUrlEncoder().withoutPadding().encodeToString(randomBytes);
        String unique = UUID.randomUUID().toString().replace("-", "");
        return random + unique;
    }

    public static String generateSessionId(String prefix) {
        String safePrefix = (prefix == null) ? "" : prefix;
        return safePrefix + generateSessionId();
    }

    public static void main(String[] args) {
        String sessionId = generateSessionId();
        System.out.println(sessionId);
    }
}