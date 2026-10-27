import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

public class AdminPasswordValidator {

    private static final int MIN_LENGTH = 12;
    private static final int MAX_LENGTH = 128;
    private static final String SPECIAL_CHARS = "!@#$%^&*()-_=+[]{};:'\",.<>/?\\|`~";
    private static final String[] FORBIDDEN_WORDS = {"admin", "password", "root", "login", "123456"};

    public static List<String> validateAdminPassword(String password) {
        List<String> errors = new ArrayList<>();

        if (password == null) {
            errors.add("Password must not be null.");
            return errors;
        }

        if (password.length() < MIN_LENGTH) {
            errors.add("Password must be at least " + MIN_LENGTH + " characters long.");
        }

        if (password.length() > MAX_LENGTH) {
            errors.add("Password must not exceed " + MAX_LENGTH + " characters.");
        }

        boolean hasUpper = false;
        boolean hasLower = false;
        boolean hasDigit = false;
        boolean hasSpecial = false;
        boolean hasWhitespace = false;

        for (char c : password.toCharArray()) {
            if (Character.isUpperCase(c)) {
                hasUpper = true;
            } else if (Character.isLowerCase(c)) {
                hasLower = true;
            } else if (Character.isDigit(c)) {
                hasDigit = true;
            } else if (SPECIAL_CHARS.indexOf(c) >= 0) {
                hasSpecial = true;
            }
            if (Character.isWhitespace(c)) {
                hasWhitespace = true;
            }
        }

        if (!hasUpper) {
            errors.add("Password must contain at least one uppercase letter.");
        }
        if (!hasLower) {
            errors.add("Password must contain at least one lowercase letter.");
        }
        if (!hasDigit) {
            errors.add("Password must contain at least one digit.");
        }
        if (!hasSpecial) {
            errors.add("Password must contain at least one special character.");
        }
        if (hasWhitespace) {
            errors.add("Password must not contain whitespace.");
        }

        if (hasRepeatedCharacters(password, 3)) {
            errors.add("Password must not contain 3 or more identical characters in a row.");
        }

        String lower = password.toLowerCase(Locale.ROOT);
        for (String word : FORBIDDEN_WORDS) {
            if (lower.contains(word)) {
                errors.add("Password must not contain the word '" + word + "'.");
            }
        }

        if (hasSequentialCharacters(password, 3)) {
            errors.add("Password must not contain 3 or more sequential characters (e.g., 'abc' or '321').");
        }

        return errors;
    }

    public static boolean isValidAdminPassword(String password) {
        return validateAdminPassword(password).isEmpty();
    }

    private static boolean hasRepeatedCharacters(String password, int run) {
        int count = 1;
        for (int i = 1; i < password.length(); i++) {
            if (password.charAt(i) == password.charAt(i - 1)) {
                count++;
                if (count >= run) {
                    return true;
                }
            } else {
                count = 1;
            }
        }
        return false;
    }

    private static boolean hasSequentialCharacters(String password, int run) {
        int ascCount = 1;
        int descCount = 1;
        for (int i = 1; i < password.length(); i++) {
            int diff = password.charAt(i) - password.charAt(i - 1);
            if (diff == 1) {
                ascCount++;
            } else {
                ascCount = 1;
            }
            if (diff == -1) {
                descCount++;
            } else {
                descCount = 1;
            }
            if (ascCount >= run || descCount >= run) {
                return true;
            }
        }
        return false;
    }

    public static String checkPasswordReport(String password) {
        List<String> errors = validateAdminPassword(password);
        if (errors.isEmpty()) {
            return "VALID: Password meets all admin requirements.";
        }
        StringBuilder sb = new StringBuilder("INVALID: Password failed the following checks:");
        for (String error : errors) {
            sb.append("\n - ").append(error);
        }
        return sb.toString();
    }

    public static void main(String[] args) {
        if (args.length == 0) {
            System.out.println("Usage: java AdminPasswordValidator <password> [more passwords...]");
            return;
        }
        for (String pwd : args) {
            System.out.println("Password: " + pwd);
            System.out.println(checkPasswordReport(pwd));
            System.out.println();
        }
    }
}