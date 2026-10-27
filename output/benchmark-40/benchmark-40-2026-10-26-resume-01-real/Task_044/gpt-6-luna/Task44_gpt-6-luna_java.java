import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.Locale;

public class Main {
    private static final int MAX_INPUT_LENGTH = 10_000;
    private static final int MAX_MENU_LENGTH = 10;

    public static void main(String[] args) throws IOException {
        BufferedReader input = new BufferedReader(new InputStreamReader(System.in));

        while (true) {
            System.out.println("Choose an operation:");
            System.out.println("1. Convert to uppercase");
            System.out.println("2. Convert to lowercase");
            System.out.println("3. Reverse text");
            System.out.println("4. Count characters");
            System.out.println("5. Count words");
            System.out.println("6. Exit");
            System.out.print("> ");

            String choice;
            try {
                choice = readBoundedLine(input, MAX_MENU_LENGTH);
            } catch (IllegalArgumentException e) {
                System.out.println("Invalid choice: input is too long.");
                continue;
            }

            if (choice == null || choice.equals("6")) {
                return;
            }

            if (!isValidChoice(choice)) {
                System.out.println("Invalid choice. Enter a number from 1 to 6.");
                continue;
            }

            System.out.print("Enter text: ");
            String text;
            try {
                text = readBoundedLine(input, MAX_INPUT_LENGTH);
            } catch (IllegalArgumentException e) {
                System.out.println("Invalid input: text exceeds the maximum length.");
                continue;
            }

            if (text == null) {
                return;
            }
            if (!isSafeText(text)) {
                System.out.println("Invalid input: control characters are not allowed.");
                continue;
            }

            switch (choice) {
                case "1":
                    System.out.println(toUpperCase(text));
                    break;
                case "2":
                    System.out.println(toLowerCase(text));
                    break;
                case "3":
                    System.out.println(reverse(text));
                    break;
                case "4":
                    System.out.println(countCharacters(text));
                    break;
                case "5":
                    System.out.println(countWords(text));
                    break;
                default:
                    System.out.println("Invalid choice.");
            }
        }
    }

    private static String readBoundedLine(BufferedReader input, int maximumLength) throws IOException {
        StringBuilder result = new StringBuilder();
        boolean tooLong = false;
        int value;

        while ((value = input.read()) != -1 && value != '\n') {
            if (result.length() < maximumLength) {
                result.append((char) value);
            } else {
                tooLong = true;
            }
        }

        if (value == -1 && result.length() == 0 && !tooLong) {
            return null;
        }

        if (tooLong) {
            throw new IllegalArgumentException("Input exceeds the maximum length.");
        }

        int length = result.length();
        if (length > 0 && result.charAt(length - 1) == '\r') {
            result.setLength(length - 1);
        }
        return result.toString();
    }

    private static boolean isValidChoice(String choice) {
        return choice.equals("1")
                || choice.equals("2")
                || choice.equals("3")
                || choice.equals("4")
                || choice.equals("5")
                || choice.equals("6");
    }

    private static boolean isSafeText(String text) {
        for (int i = 0; i < text.length(); i++) {
            char current = text.charAt(i);
            if (Character.isISOControl(current)) {
                return false;
            }
            if (Character.isHighSurrogate(current)) {
                if (i + 1 >= text.length() || !Character.isLowSurrogate(text.charAt(i + 1))) {
                    return false;
                }
                i++;
            } else if (Character.isLowSurrogate(current)) {
                return false;
            }
        }
        return true;
    }

    private static String toUpperCase(String text) {
        return text.toUpperCase(Locale.ROOT);
    }

    private static String toLowerCase(String text) {
        return text.toLowerCase(Locale.ROOT);
    }

    private static String reverse(String text) {
        return new StringBuilder(text).reverse().toString();
    }

    private static int countCharacters(String text) {
        return text.codePointCount(0, text.length());
    }

    private static int countWords(String text) {
        int words = 0;
        boolean insideWord = false;

        for (int offset = 0; offset < text.length();) {
            int codePoint = text.codePointAt(offset);
            boolean whitespace = Character.isWhitespace(codePoint);

            if (!whitespace && !insideWord) {
                words++;
            }
            insideWord = !whitespace;
            offset += Character.charCount(codePoint);
        }

        return words;
    }
}