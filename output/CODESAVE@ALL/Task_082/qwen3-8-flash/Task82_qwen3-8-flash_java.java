import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

public class Main {
    public static String hashPasswordWithSalt(String password, String salt) {
        String normalizedPassword = password == null ? "" : password;
        String normalizedSalt = salt == null ? "" : salt;

        try {
            MessageDigest digest = MessageDigest.getInstance("SHA-256");
            byte[] hash = digest.digest((normalizedSalt + normalizedPassword).getBytes(StandardCharsets.UTF_8));

            StringBuilder hexString = new StringBuilder(hash.length * 2);
            for (byte b : hash) {
                String hex = Integer.toHexString(b & 0xff);
                if (hex.length() == 1) {
                    hexString.append('0');
                }
                hexString.append(hex);
            }

            return hexString.toString();
        } catch (NoSuchAlgorithmException e) {
            throw new RuntimeException("SHA-256 algorithm is not available", e);
        }
    }

    public static void main(String[] args) {
        if (args.length >= 2) {
            System.out.println(hashPasswordWithSalt(args[0], args[1]));
        } else {
            System.out.println(hashPasswordWithSalt("password", "salt"));
        }
    }
}