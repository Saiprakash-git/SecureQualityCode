import java.util.Scanner;

public class SafeStringProcessor {

    public static String processString(String input) {
        if (input == null) {
            return "";
        }

        String trimmed = input.trim();

        if (trimmed.isEmpty()) {
            return "";
        }

        StringBuilder sb = new StringBuilder(trimmed.length());
        for (int i = trimmed.length() - 1; i >= 0; i--) {
            sb.append(trimmed.charAt(i));
        }

        return sb.toString();
    }

    public static boolean isValidInput(String input, int maxLength) {
        if (input == null) {
            return false;
        }
        return input.length() <= maxLength;
    }

    public static void main(String[] args) {
        final int MAX_INPUT_LENGTH = 1000;

        try (Scanner scanner = new Scanner(System.in)) {
            System.out.print("Enter a string: ");

            if (scanner.hasNextLine()) {
                String userInput = scanner.nextLine();

                if (!isValidInput(userInput, MAX_INPUT_LENGTH)) {
                    System.out.println("Error: Input exceeds maximum length of " + MAX_INPUT_LENGTH + " characters.");
                    return;
                }

                String result = processString(userInput);
                System.out.println("Processed result: " + result);
            }
        }
    }
}