import java.util.ArrayList;
import java.util.List;

public class AdminPasswordValidator {

    private static final int MIN_LENGTH = 12;
    private static final int MAX_LENGTH = 128;
    private static final String SPECIAL_CHARACTERS = "!@#$%^&*()_+-=[]{}|;':\",./<>?`~\\";

    public static class ValidationResult {
        private final boolean valid;
        private final List<String> errors;

        public ValidationResult(boolean valid, List<String> errors) {
            this.valid = valid;
            this.errors = errors;
        }

        public boolean isValid() {
            return valid;
        }

        public List<String> getErrors() {
            return errors;
        }

        @Override
        public String toString() {
            return "Valid: " + valid + (errors.isEmpty() ? "" : ", Errors: " + errors);
        }
    }

    public static boolean isValidAdminPassword(String password) {
        return validateAdminPassword(password).isValid();
    }

    public static ValidationResult validateAdminPassword(String password) {
        List<String> errors = new ArrayList<>();

        if (password == null) {
            errors.add("Password cannot be null.");
            return new ValidationResult(false, errors);
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

        for (char ch : password.toCharArray()) {
            if (Character.isWhitespace(ch)) {
                hasWhitespace = true;
            } else if (Character.isUpperCase(ch)) {
                hasUpper = true;
            } else if (Character.isLowerCase(ch)) {
                hasLower = true;
            } else if (Character.isDigit(ch)) {
                hasDigit = true;
            } else if (SPECIAL_CHARACTERS.indexOf(ch) >= 0) {
                hasSpecial = true;
            }
        }

        if (hasWhitespace) {
            errors.add("Password must not contain any whitespace characters.");
        }
        if (!hasUpper) {
            errors.add("Password must contain at least one uppercase letter.");
        }
        if (!hasLower) {
            errors.add("Password must contain at least one lowercase letter.");
        }
        if (!hasDigit) {
            errors.add("Password must contain at least one numeric digit.");
        }
        if (!hasSpecial) {
            errors.add("Password must contain at least one special character.");
        }

        String lower = password.toLowerCase();
        if (lower.contains("admin") || lower.contains("password") || lower.contains("root")) {
            errors.add("Password must not contain predictable keywords like 'admin', 'password', or 'root'.");
        }

        return new ValidationResult(errors.isEmpty(), errors);
    }

    public static void main(String[] args) {
        String[] testPasswords = {
            "Weak1!",
            "admin123456!A",
            "SuperS3cure#AdminPass!",
            "ValidAdmin@Pass2026",
            "invalid password with spaces 123!A",
            "Secure#G4teKeeper2026!"
        };

        for (String pwd : testPasswords) {
            ValidationResult result = validateAdminPassword(pwd);
            System.out.println("Password: \"" + pwd + "\"");
            System.out.println("Status: " + (result.isValid() ? "ACCEPTED" : "REJECTED"));
            if (!result.isValid()) {
                System.out.println("Reasons: " + result.getErrors());
            }
            System.out.println();
        }
    }
}