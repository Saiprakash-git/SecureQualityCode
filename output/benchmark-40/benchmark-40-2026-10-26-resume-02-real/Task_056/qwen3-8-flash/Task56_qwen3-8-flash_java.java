import java.nio.charset.StandardCharsets;
import java.security.GeneralSecurityException;
import java.util.Base64;
import javax.crypto.Mac;
import javax.crypto.spec.SecretKeySpec;

public class Main {
    public static void main(String[] args) throws GeneralSecurityException {
        String username = args.length > 0 ? args[0] : "alice";
        String password = args.length > 1 ? args[1] : "password";
        long issuedAtMillis = System.currentTimeMillis();
        long expiresAtMillis = issuedAtMillis + 3600000L;

        String token = generateAuthenticationToken(username, password, issuedAtMillis, expiresAtMillis);
        System.out.println(token);
    }

    public static String generateAuthenticationToken(String username, String password, long issuedAtMillis, long expiresAtMillis) throws GeneralSecurityException {
        if (username == null || username.isEmpty()) {
            throw new IllegalArgumentException("username must not be empty");
        }

        if (password == null || password.isEmpty()) {
            throw new IllegalArgumentException("password must not be empty");
        }

        if (expiresAtMillis <= issuedAtMillis) {
            throw new IllegalArgumentException("expiresAtMillis must be greater than issuedAtMillis");
        }

        String payload = createTokenPayload(username, issuedAtMillis, expiresAtMillis);
        String encodedPayload = encodeBase64Url(payload);
        String signature = signPayload(encodedPayload, password);

        return encodedPayload + "." + signature;
    }

    public static String createTokenPayload(String username, long issuedAtMillis, long expiresAtMillis) {
        return username + "|" + issuedAtMillis + "|" + expiresAtMillis;
    }

    public static String encodeBase64Url(String value) {
        return Base64.getUrlEncoder().withoutPadding().encodeToString(value.getBytes(StandardCharsets.UTF_8));
    }

    public static String signPayload(String payload, String secret) throws GeneralSecurityException {
        Mac mac = Mac.getInstance("HmacSHA256");
        mac.init(new SecretKeySpec(secret.getBytes(StandardCharsets.UTF_8), "HmacSHA256"));

        byte[] signatureBytes = mac.doFinal(payload.getBytes(StandardCharsets.UTF_8));
        return Base64.getUrlEncoder().withoutPadding().encodeToString(signatureBytes);
    }
}