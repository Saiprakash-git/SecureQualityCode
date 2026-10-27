import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.text.Normalizer;
import java.util.Scanner;
import java.util.regex.Pattern;

public class SecureInputProcessor {

    private static final int MAX_INPUT_LENGTH = 256;
    private static final int MIN_AGE = 1;
    private static final int MAX_AGE = 125;

    private static final Pattern USERNAME_PATTERN = Pattern.compile("^[a-zA-Z0-9_-]{3,20}$");
    private static final Pattern EMAIL_PATTERN = Pattern.compile("^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,63}$");
    private static final Pattern SAFE_TEXT_PATTERN = Pattern.compile("^[\\p{L}\\p{N}\\p{P}\\p{Z}]{1,100}$");

    public static void main(String[] args) {
        Scanner scanner = new Scanner(System.in);

        System.out.println("=== Secure Data Processing System ===");

        try {
            System.out.print("Enter username (3-20 chars, alphanumeric, '_', '-'): ");
            String rawUsername = scanner.nextLine();
            String username = validateUsername(rawUsername);

            System.out.print("Enter email address: ");
            String rawEmail = scanner.nextLine();
            String email = validateEmail(rawEmail);

            System.out.print("Enter age (1-125): ");
            String rawAge = scanner.nextLine();
            int age = validateAge(rawAge);

            System.out.print("Enter a brief description/note (max 100 chars): ");
            String rawNote = scanner.nextLine();
            String sanitizedNote = sanitizeAndValidateText(rawNote);

            String result = processUserData(username, email, age, sanitizedNote);
            System.out.println("\n=== Processing Result ===");
            System.out.println(result);

        } catch (IllegalArgumentException e) {
            System.err.println("Security/Validation Error: " + e.getMessage());
        } finally {
            scanner.close();
        }
    }

    public static String validateUsername(String input) {
        if (input == null) {
            throw new IllegalArgumentException("Username cannot be null.");
        }
        String trimmed = input.trim();
        if (trimmed.length() > MAX_INPUT_LENGTH) {
            throw new IllegalArgumentException("Username exceeds maximum length.");
        }
        if (!USERNAME_PATTERN.matcher(trimmed).matches()) {
            throw new IllegalArgumentException("Username contains invalid characters or has invalid length.");
        }
        return trimmed;
    }

    public static String validateEmail(String input) {
        if (input == null) {
            throw new IllegalArgumentException("Email cannot be null.");
        }
        String trimmed = input.trim();
        if (trimmed.length() > MAX_INPUT_LENGTH) {
            throw new IllegalArgumentException("Email exceeds maximum length.");
        }
        if (!EMAIL_PATTERN.matcher(trimmed).matches()) {
            throw new IllegalArgumentException("Email format is invalid.");
        }
        return trimmed.toLowerCase();
    }

    public static int validateAge(String input) {
        if (input == null) {
            throw new IllegalArgumentException("Age cannot be null.");
        }
        String trimmed = input.trim();
        if (trimmed.length() > 3) {
            throw new IllegalArgumentException("Age input is out of valid bounds.");
        }

        int age;
        try {
            age = Integer.parseInt(trimmed);
        } catch (NumberFormatException e) {
            throw new IllegalArgumentException("Age must be a valid integer.");
        }

        if (age < MIN_AGE || age > MAX_AGE) {
            throw new IllegalArgumentException("Age must be between " + MIN_AGE + " and " + MAX_AGE + ".");
        }
        return age;
    }

    public static String sanitizeAndValidateText(String input) {
        if (input == null) {
            throw new IllegalArgumentException("Note cannot be null.");
        }

        String normalized = Normalizer.normalize(input, Normalizer.Form.NFKC).trim();

        if (normalized.length() > MAX_INPUT_LENGTH) {
            throw new IllegalArgumentException("Input text exceeds maximum allowed buffer length.");
        }

        if (!SAFE_TEXT_PATTERN.matcher(normalized).matches()) {
            throw new IllegalArgumentException("Input contains disallowed characters.");
        }

        return escapeHtml(normalized);
    }

    public static String escapeHtml(String input) {
        if (input == null) {
            return "";
        }
        StringBuilder out = new StringBuilder(Math.max(16, input.length()));
        for (int i = 0; i < input.length(); i++) {
            char c = input.charAt(i);
            switch (c) {
                case '<':
                    out.append("&lt;");
                    break;
                case '>':
                    out.append("&gt;");
                    break;
                case '&':
                    out.append("&amp;");
                    break;
                case '"':
                    out.append("&quot;");
                    break;
                case '\'':
                    out.append("&#x27;");
                    break;
                case '/':
                    out.append("&#x2F;");
                    break;
                default:
                    out.append(c);
            }
        }
        return out.toString();
    }

    public static String computeSha256(String input) {
        if (input == null) {
            throw new IllegalArgumentException("Cannot hash null content.");
        }
        try {
            MessageDigest digest = MessageDigest.getInstance("SHA-256");
            byte[] hashBytes = digest.digest(input.getBytes(StandardCharsets.UTF_8));
            StringBuilder hexString = new StringBuilder();
            for (byte b : hashBytes) {
                String hex = Integer.toHexString(0xff & b);
                if (hex.length() == 1) {
                    hexString.append('0');
                }
                hexString.append(hex);
            }
            return hexString.toString();
        } catch (NoSuchAlgorithmException e) {
            throw new IllegalStateException("Cryptographic algorithm not available.", e);
        }
    }

    public static String processUserData(String username, String email, int age, String sanitizedNote) {
        String emailHash = computeSha256(email);
        int birthYearEstimate = java.time.Year.now().getValue() - age;

        return String.format(
            "Profile Summary:\n" +
            "  Username: %s\n" +
            "  Email Hash (SHA-256): %s\n" +
            "  Age: %d (Estimated Birth Year: %d)\n" +
            "  Sanitized Note: %s\n" +
            "  Status: Validated and securely processed.",
            username, emailHash, age, birthYearEstimate, sanitizedNote
        );
    }
}