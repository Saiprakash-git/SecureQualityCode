import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.regex.Pattern;

public class Main {
    private static final int MAX_INPUT_LENGTH = 1024;
    private static final Pattern SAFE_INPUT_PATTERN = Pattern.compile("[A-Za-z0-9 .,!?;:'\"-]{1,1024}");

    public static void main(String[] args) throws IOException {
        BufferedReader reader = new BufferedReader(new InputStreamReader(System.in, StandardCharsets.UTF_8));
        String input = reader.readLine();
        System.out.println(processInput(input));
    }

    public static String processInput(String input) {
        try {
            String sanitized = validateAndSanitize(input);

            StringBuilder output = new StringBuilder();
            output.append("Sanitized input: ").append(sanitized).append(System.lineSeparator());
            output.append("Character count: ").append(sanitized.length()).append(System.lineSeparator());
            output.append("Word count: ").append(countWords(sanitized)).append(System.lineSeparator());
            output.append("Reversed input: ").append(reverse(sanitized)).append(System.lineSeparator());
            output.append("SHA-256 hash: ").append(sha256Hex(sanitized));

            return output.toString();
        } catch (IllegalArgumentException e) {
            return "Invalid input: " + e.getMessage();
        } catch (Exception e) {
            return "An error occurred while processing input.";
        }
    }

    public static String validateAndSanitize(String input) {
        if (input == null) {
            throw new IllegalArgumentException("Input is missing.");
        }

        String trimmed = input.trim();

        if (trimmed.isEmpty()) {
            throw new IllegalArgumentException("Input is empty.");
        }

        if (trimmed.length() > MAX_INPUT_LENGTH) {
            throw new IllegalArgumentException("Input is too long.");
        }

        if (trimmed.indexOf('\0') >= 0) {
            throw new IllegalArgumentException("Input contains invalid control characters.");
        }

        if (!SAFE_INPUT_PATTERN.matcher(trimmed).matches()) {
            throw new IllegalArgumentException("Input contains unsupported characters.");
        }

        return trimmed;
    }

    public static int countWords(String input) {
        if (input == null || input.isEmpty()) {
            return 0;
        }

        String[] words = input.trim().split("\\s+");
        return words.length;
    }

    public static String reverse(String input) {
        if (input == null) {
            return "";
        }

        return new StringBuilder(input).reverse().toString();
    }

    public static String sha256Hex(String input) throws NoSuchAlgorithmException {
        MessageDigest digest = MessageDigest.getInstance("SHA-256");
        byte[] hash = digest.digest(input.getBytes(StandardCharsets.UTF_8));

        StringBuilder hex = new StringBuilder(hash.length * 2);
        for (byte b : hash) {
            String part = Integer.toHexString(b & 0xff);
            if (part.length() == 1) {
                hex.append('0');
            }
            hex.append(part);
        }

        return hex.toString();
    }
}