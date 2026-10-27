import java.security.SecureRandom;
import java.util.Base64;
import java.util.Map;
import java.util.HashMap;
import java.nio.charset.StandardCharsets;
import javax.crypto.Mac;
import javax.crypto.spec.SecretKeySpec;

public class AuthTokenGenerator {

    private static final SecureRandom RANDOM = new SecureRandom();
    private static final String HMAC_ALGORITHM = "HmacSHA256";

    public static String generateToken(String userId, String secretKey, long ttlSeconds) throws Exception {
        if (userId == null || userId.isEmpty()) {
            throw new IllegalArgumentException("userId must not be empty");
        }
        if (secretKey == null || secretKey.isEmpty()) {
            throw new IllegalArgumentException("secretKey must not be empty");
        }
        long issuedAt = System.currentTimeMillis() / 1000L;
        long expiresAt = issuedAt + ttlSeconds;
        byte[] nonceBytes = new byte[16];
        RANDOM.nextBytes(nonceBytes);
        String nonce = Base64.getUrlEncoder().withoutPadding().encodeToString(nonceBytes);

        String payload = userId + "|" + issuedAt + "|" + expiresAt + "|" + nonce;
        String encodedPayload = Base64.getUrlEncoder().withoutPadding()
                .encodeToString(payload.getBytes(StandardCharsets.UTF_8));
        String signature = sign(encodedPayload, secretKey);
        return encodedPayload + "." + signature;
    }

    public static String sign(String data, String secretKey) throws Exception {
        Mac mac = Mac.getInstance(HMAC_ALGORITHM);
        SecretKeySpec keySpec = new SecretKeySpec(secretKey.getBytes(StandardCharsets.UTF_8), HMAC_ALGORITHM);
        mac.init(keySpec);
        byte[] raw = mac.doFinal(data.getBytes(StandardCharsets.UTF_8));
        return Base64.getUrlEncoder().withoutPadding().encodeToString(raw);
    }

    public static Map<String, Object> validateToken(String token, String secretKey) throws Exception {
        Map<String, Object> result = new HashMap<>();
        if (token == null || secretKey == null) {
            result.put("valid", false);
            result.put("reason", "missing token or key");
            return result;
        }
        int dot = token.lastIndexOf('.');
        if (dot <= 0 || dot == token.length() - 1) {
            result.put("valid", false);
            result.put("reason", "malformed token");
            return result;
        }
        String encodedPayload = token.substring(0, dot);
        String providedSig = token.substring(dot + 1);
        String expectedSig = sign(encodedPayload, secretKey);

        if (!constantTimeEquals(expectedSig, providedSig)) {
            result.put("valid", false);
            result.put("reason", "invalid signature");
            return result;
        }

        String payload = new String(Base64.getUrlDecoder().decode(encodedPayload), StandardCharsets.UTF_8);
        String[] parts = payload.split("\\|");
        if (parts.length != 4) {
            result.put("valid", false);
            result.put("reason", "malformed payload");
            return result;
        }
        long expiresAt = Long.parseLong(parts[2]);
        long now = System.currentTimeMillis() / 1000L;
        if (now >= expiresAt) {
            result.put("valid", false);
            result.put("reason", "token expired");
            return result;
        }
        result.put("valid", true);
        result.put("userId", parts[0]);
        result.put("issuedAt", Long.parseLong(parts[1]));
        result.put("expiresAt", expiresAt);
        return result;
    }

    private static boolean constantTimeEquals(String a, String b) {
        if (a.length() != b.length()) {
            return false;
        }
        int diff = 0;
        for (int i = 0; i < a.length(); i++) {
            diff |= a.charAt(i) ^ b.charAt(i);
        }
        return diff == 0;
    }

    public static void main(String[] args) throws Exception {
        String userId = args.length > 0 ? args[0] : "user123";
        String secret = args.length > 1 ? args[1] : "change-this-secret-key";
        long ttl = args.length > 2 ? Long.parseLong(args[2]) : 3600L;

        String token = generateToken(userId, secret, ttl);
        System.out.println("Token: " + token);
        System.out.println("Validation: " + validateToken(token, secret));
    }
}