import java.util.UUID;

public final class SessionIdGenerator {
    private SessionIdGenerator() {
    }

    public static String generateSessionId() {
        return UUID.randomUUID().toString();
    }

    public static void main(String[] args) {
        System.out.println(generateSessionId());
    }
}