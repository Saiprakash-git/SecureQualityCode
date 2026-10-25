import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.Locale;

public class Main {
    private static final int MAX_INPUT_LENGTH = 1_000_000;

    public static void main(String[] args) throws IOException {
        System.out.print("Enter a string: ");
        BufferedReader reader = new BufferedReader(new InputStreamReader(System.in));

        try {
            String input = readBoundedLine(reader, MAX_INPUT_LENGTH);
            if (input == null) {
                System.out.println("No input provided.");
                return;
            }
            System.out.println("Processed string: " + process(input));
        } catch (IllegalArgumentException e) {
            System.out.println("Input exceeds the maximum allowed length of "
                    + MAX_INPUT_LENGTH + " characters.");
        }
    }

    private static String readBoundedLine(BufferedReader reader, int maxLength)
            throws IOException {
        StringBuilder input = new StringBuilder();
        int character;

        while ((character = reader.read()) != -1 && character != '\n') {
            if (input.length() >= maxLength) {
                throw new IllegalArgumentException("Input too long");
            }
            if (character != '\r') {
                input.append((char) character);
            }
        }

        if (character == -1 && input.length() == 0) {
            return null;
        }
        return input.toString();
    }

    private static String process(String input) {
        return input.trim().toUpperCase(Locale.ROOT);
    }
}