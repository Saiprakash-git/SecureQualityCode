import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;

public class SafeStringProcessor {

    private static final int MAX_INPUT_LENGTH = 1024;

    public static String readBoundedLine(BufferedReader reader, int maxLength) throws IOException {
        StringBuilder sb = new StringBuilder();
        int ch;
        while ((ch = reader.read()) != -1) {
            if (ch == '\n') {
                break;
            }
            if (ch == '\r') {
                continue;
            }
            if (sb.length() >= maxLength) {
                throw new IOException("Input exceeds maximum allowed length of " + maxLength + " characters.");
            }
            sb.append((char) ch);
        }
        if (ch == -1 && sb.length() == 0) {
            return null;
        }
        return sb.toString();
    }

    public static String processString(String input) {
        if (input == null) {
            return "";
        }
        String trimmed = input.trim();
        String reversed = new StringBuilder(trimmed).reverse().toString();
        String upper = trimmed.toUpperCase();
        int vowels = 0;
        for (int i = 0; i < trimmed.length(); i++) {
            switch (Character.toLowerCase(trimmed.charAt(i))) {
                case 'a': case 'e': case 'i': case 'o': case 'u':
                    vowels++;
                    break;
                default:
                    break;
            }
        }
        int wordCount = trimmed.isEmpty() ? 0 : trimmed.split("\\s+").length;

        StringBuilder result = new StringBuilder();
        result.append("Original length: ").append(input.length()).append('\n');
        result.append("Trimmed: ").append(trimmed).append('\n');
        result.append("Uppercase: ").append(upper).append('\n');
        result.append("Reversed: ").append(reversed).append('\n');
        result.append("Vowel count: ").append(vowels).append('\n');
        result.append("Word count: ").append(wordCount);
        return result.toString();
    }

    public static void main(String[] args) {
        BufferedReader reader = new BufferedReader(
                new InputStreamReader(System.in, StandardCharsets.UTF_8));
        try {
            System.out.print("Enter a string (max " + MAX_INPUT_LENGTH + " characters): ");
            System.out.flush();
            String input = readBoundedLine(reader, MAX_INPUT_LENGTH);
            if (input == null) {
                System.out.println("No input provided.");
                return;
            }
            System.out.println(processString(input));
        } catch (IOException e) {
            System.err.println("Error: " + e.getMessage());
            System.exit(1);
        }
    }
}