import java.security.SecureRandom;
import java.util.Base64;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;

public class Main {
    private static final SecureRandom RANDOM = new SecureRandom();
    private static final Set<String> ISSUED_SESSION_IDS = ConcurrentHashMap.newKeySet();

    public static String generateSessionId(int byteLength) {
        int length = byteLength <= 0 ? 32 : byteLength;

        while (true) {
            byte[] bytes = new byte[length];
            RANDOM.nextBytes(bytes);

            String sessionId = Base64.getUrlEncoder().withoutPadding().encodeToString(bytes);

            if (ISSUED_SESSION_IDS.add(sessionId)) {
                return sessionId;
            }
        }
    }

    public static void main(String[] args) {
        System.out.println(generateSessionId(32));
    }
}