import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.nio.charset.StandardCharsets;

public class PasswordHasher {
    public static String hashPasswordWithSalt(String password, String salt) throws NoSuchAlgorithmException {
        MessageDigest digest = MessageDigest.getInstance("SHA-256");
        String combined = password + salt;
        byte[] hashBytes = digest.digest(combined.getBytes(StandardCharsets.UTF_8));
        StringBuilder hexString = new StringBuilder();
        for (byte b : hashBytes) {
            String hex = Integer.toHexString(0xff & b);
            if (hex.length() == 1) {
                hexString.append('0');
            }
            hexString.append(hex);
        }
        return hexString.toString();
    }

    public static void main(String[] args) throws NoSuchAlgorithmException {
        if (args.length == 2) {
            System.out.println(hashPasswordWithSalt(args[0], args[1]));
        }
    }
}