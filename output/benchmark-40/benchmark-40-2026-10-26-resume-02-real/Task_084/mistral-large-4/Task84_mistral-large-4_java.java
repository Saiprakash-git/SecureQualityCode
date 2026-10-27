import java.security.SecureRandom;
import java.util.UUID;

public class SessionIdGenerator {

    public static String generateSessionId(int length) {
        if (length < 1) {
            throw new IllegalArgumentException("Length must be at least 1");
        }
        SecureRandom secureRandom = new SecureRandom();
        String characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
        StringBuilder sessionId = new StringBuilder(length);
        for (int i = 0; i < length; i++) {
            sessionId.append(characters.charAt(secureRandom.nextInt(characters.length())));
        }
        return sessionId.toString();
    }

    public static String generateSessionId() {
        return UUID.randomUUID().toString().replace("-", "");
    }

    public static void main(String[] args) {
        int length = 32;
        if (args.length > 0) {
            try {
                length = Integer.parseInt(args[0]);
            } catch (NumberFormatException e) {
                length = 32;
            }
        }
        String sessionId = generateSessionId(length);
        System.out.println(sessionId);
    }
}