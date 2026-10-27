import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.regex.Pattern;

public class AdminPasswordValidator {

    private static final int MIN_LENGTH = 12;
    private static final Pattern UPPER_CASE = Pattern.compile("[A-Z]");
    private static final Pattern LOWER_CASE = Pattern.compile("[a-z]");
    private static final Pattern DIGIT = Pattern.compile("[0-9]");
    private static final Pattern SPECIAL_CHAR = Pattern.compile("[^a-zA-Z0-9]");

    public static boolean validatePasswordStrength(String password) {
        if (password == null || password.length() < MIN_LENGTH) {
            return false;
        }
        if (!UPPER_CASE.matcher(password).find()) {
            return false;
        }
        if (!LOWER_CASE.matcher(password).find()) {
            return false;
        }
        if (!DIGIT.matcher(password).find()) {
            return false;
        }
        if (!SPECIAL_CHAR.matcher(password).find()) {
            return false;
        }
        return true;
    }

    public static boolean checkAdminPassword(String inputPassword, String storedHash) {
        if (inputPassword == null || storedHash == null) {
            return false;
        }
        if (!validatePasswordStrength(inputPassword)) {
            return false;
        }
        String inputHash = hashPassword(inputPassword);
        return inputHash.equals(storedHash);
    }

    private static String hashPassword(String password) {
        try {
            MessageDigest md = MessageDigest.getInstance("SHA-256");
            byte[] hashBytes = md.digest(password.getBytes());
            StringBuilder sb = new StringBuilder();
            for (byte b : hashBytes) {
                sb.append(String.format("%02x", b));
            }
            return sb.toString();
        } catch (NoSuchAlgorithmException e) {
            throw new RuntimeException("SHA-256 algorithm not available", e);
        }
    }

    public static void main(String[] args) {
        String password = "SecureAdmin123!";
        String storedHash = hashPassword("SecureAdmin123!");
        boolean isValid = checkAdminPassword(password, storedHash);
        System.out.println(isValid);
    }
}