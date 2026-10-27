import java.nio.charset.StandardCharsets;
import java.security.InvalidKeyException;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;
import java.time.Instant;
import java.util.Base64;
import javax.crypto.Mac;
import javax.crypto.spec.SecretKeySpec;

public class AuthTokenGenerator {

    private static final String HMAC_ALGORITHM = "HmacSHA256";

    public static byte[] generateSecureSecret(int byteLength) {
        byte[] secret = new byte[byteLength];
        new SecureRandom().nextBytes(secret);
        return secret;
    }

    public static String generateOpaqueToken(int byteLength) {
        byte[] randomBytes = new byte[byteLength];
        new SecureRandom().nextBytes(randomBytes);
        return Base64.getUrlEncoder().withoutPadding().encodeToString(randomBytes);
    }

    public static String generateJwtToken(String userId, long validitySeconds, byte[] secretKey) {
        long issuedAt = Instant.now().getEpochSecond();
        long expiresAt = issuedAt + validitySeconds;

        String headerJson = "{\"alg\":\"HS256\",\"typ\":\"JWT\"}";
        String payloadJson = String.format("{\"sub\":\"%s\",\"iat\":%d,\"exp\":%d}", 
                escapeJson(userId), issuedAt, expiresAt);

        Base64.Encoder encoder = Base64.getUrlEncoder().withoutPadding();
        String encodedHeader = encoder.encodeToString(headerJson.getBytes(StandardCharsets.UTF_8));
        String encodedPayload = encoder.encodeToString(payloadJson.getBytes(StandardCharsets.UTF_8));

        String contentToSign = encodedHeader + "." + encodedPayload;
        byte[] signatureBytes = computeHmacSha256(contentToSign.getBytes(StandardCharsets.UTF_8), secretKey);
        String encodedSignature = encoder.encodeToString(signatureBytes);

        return contentToSign + "." + encodedSignature;
    }

    public static boolean validateJwtToken(String token, byte[] secretKey) {
        String[] parts = token.split("\\.");
        if (parts.length != 3) {
            return false;
        }

        String encodedHeader = parts[0];
        String encodedPayload = parts[1];
        String encodedSignature = parts[2];

        String contentToSign = encodedHeader + "." + encodedPayload;
        byte[] expectedSignature = computeHmacSha256(contentToSign.getBytes(StandardCharsets.UTF_8), secretKey);
        byte[] actualSignature;

        try {
            actualSignature = Base64.getUrlDecoder().decode(encodedSignature);
        } catch (IllegalArgumentException e) {
            return false;
        }

        if (!MessageDigest.isEqual(expectedSignature, actualSignature)) {
            return false;
        }

        try {
            String payloadJson = new String(Base64.getUrlDecoder().decode(encodedPayload), StandardCharsets.UTF_8);
            long expiresAt = extractExpiration(payloadJson);
            long currentTime = Instant.now().getEpochSecond();
            return currentTime <= expiresAt;
        } catch (Exception e) {
            return false;
        }
    }

    public static String extractSubject(String token) {
        String[] parts = token.split("\\.");
        if (parts.length != 3) {
            throw new IllegalArgumentException("Invalid token format");
        }

        String payloadJson = new String(Base64.getUrlDecoder().decode(parts[1]), StandardCharsets.UTF_8);
        int subIndex = payloadJson.indexOf("\"sub\":");
        if (subIndex == -1) {
            throw new IllegalArgumentException("Token payload missing subject");
        }

        int startQuote = payloadJson.indexOf("\"", subIndex + 6);
        int endQuote = payloadJson.indexOf("\"", startQuote + 1);
        if (startQuote == -1 || endQuote == -1) {
            throw new IllegalArgumentException("Malformed subject in token payload");
        }

        return payloadJson.substring(startQuote + 1, endQuote);
    }

    private static byte[] computeHmacSha256(byte[] data, byte[] key) {
        try {
            Mac mac = Mac.getInstance(HMAC_ALGORITHM);
            SecretKeySpec secretKeySpec = new SecretKeySpec(key, HMAC_ALGORITHM);
            mac.init(secretKeySpec);
            return mac.doFinal(data);
        } catch (NoSuchAlgorithmException | InvalidKeyException e) {
            throw new IllegalStateException("Failed to calculate HMAC-SHA256", e);
        }
    }

    private static long extractExpiration(String payloadJson) {
        int expIndex = payloadJson.indexOf("\"exp\":");
        if (expIndex == -1) {
            throw new IllegalArgumentException("Token payload missing expiration");
        }

        int start = expIndex + 6;
        int end = start;
        while (end < payloadJson.length() && Character.isDigit(payloadJson.charAt(end))) {
            end++;
        }

        return Long.parseLong(payloadJson.substring(start, end));
    }

    private static String escapeJson(String input) {
        return input.replace("\\", "\\\\")
                    .replace("\"", "\\\"")
                    .replace("\b", "\\b")
                    .replace("\f", "\\f")
                    .replace("\n", "\\n")
                    .replace("\r", "\\r")
                    .replace("\t", "\\t");
    }

    public static void main(String[] args) {
        byte[] secretKey = generateSecureSecret(32);
        String userId = "user_49182";
        long validityPeriod = 3600;

        String jwtToken = generateJwtToken(userId, validityPeriod, secretKey);
        System.out.println("Generated JWT Token: " + jwtToken);

        boolean isValid = validateJwtToken(jwtToken, secretKey);
        System.out.println("Token Valid: " + isValid);

        if (isValid) {
            String authenticatedUser = extractSubject(jwtToken);
            System.out.println("Authenticated User ID: " + authenticatedUser);
        }

        String opaqueToken = generateOpaqueToken(32);
        System.out.println("Generated Opaque Token: " + opaqueToken);
    }
}