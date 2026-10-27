import java.security.SecureRandom;
import java.util.Base64;
import java.util.UUID;
import javax.crypto.Mac;
import javax.crypto.spec.SecretKeySpec;

public class TokenGenerator {

    private static final String HMAC_ALGORITHM = "HmacSHA256";
    private static final SecureRandom SECURE_RANDOM = new SecureRandom();

    public static String generateRandomToken(int byteLength) {
        byte[] bytes = new byte[byteLength];
        SECURE_RANDOM.nextBytes(bytes);
        return Base64.getUrlEncoder().withoutPadding().encodeToString(bytes);
    }

    public static String generateUUIDToken() {
        return UUID.randomUUID().toString().replace("-", "");
    }

    public static String generateSignedToken(String payload, String secretKey) {
        try {
            Mac mac = Mac.getInstance(HMAC_ALGORITHM);
            SecretKeySpec keySpec = new SecretKeySpec(secretKey.getBytes(), HMAC_ALGORITHM);
            mac.init(keySpec);
            byte[] signature = mac.doFinal(payload.getBytes());
            String encodedSignature = Base64.getUrlEncoder().withoutPadding().encodeToString(signature);
            String encodedPayload = Base64.getUrlEncoder().withoutPadding().encodeToString(payload.getBytes());
            return encodedPayload + "." + encodedSignature;
        } catch (Exception e) {
            throw new RuntimeException("Token generation failed", e);
        }
    }

    public static boolean validateToken(String token, String secretKey) {
        try {
            String[] parts = token.split("\\.");
            if (parts.length != 2) return false;
            String payload = new String(Base64.getUrlDecoder().decode(parts[0]));
            String providedSignature = parts[1];
            String expectedToken = generateSignedToken(payload, secretKey);
            String expectedSignature = expectedToken.split("\\.")[1];
            return providedSignature.equals(expectedSignature);
        } catch (Exception e) {
            return false;
        }
    }

    public static String generateTokenWithExpiry(String userId, String secretKey, long expiryMinutes) {
        long expiryTime = System.currentTimeMillis() + (expiryMinutes * 60 * 1000);
        String payload = userId + ":" + expiryTime;
        return generateSignedToken(payload, secretKey);
    }

    public static boolean isTokenExpired(String token, String secretKey) {
        if (!validateToken(token, secretKey)) return true;
        try {
            String[] parts = token.split("\\.");
            String payload = new String(Base64.getUrlDecoder().decode(parts[0]));
            String[] payloadParts = payload.split(":");
            long expiryTime = Long.parseLong(payloadParts[1]);
            return System.currentTimeMillis() > expiryTime;
        } catch (Exception e) {
            return true;
        }
    }

    public static void main(String[] args) {
        String randomToken = generateRandomToken(32);
        System.out.println("Random Token: " + randomToken);

        String uuidToken = generateUUIDToken();
        System.out.println("UUID Token: " + uuidToken);

        String secret = "my-secret-key";
        String payload = "user123:admin";
        String signedToken = generateSignedToken(payload, secret);
        System.out.println("Signed Token: " + signedToken);

        boolean isValid = validateToken(signedToken, secret);
        System.out.println("Token Valid: " + isValid);

        String expiringToken = generateTokenWithExpiry("user456", secret, 60);
        System.out.println("Expiring Token: " + expiringToken);
        System.out.println("Is Expired: " + isTokenExpired(expiringToken, secret));
    }
}