import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;

public class Main {
    public static String processString(String input) {
        if (input == null) {
            return "";
        }

        int length = input.length();
        int uppercaseCount = 0;
        int lowercaseCount = 0;
        int digitCount = 0;
        int whitespaceCount = 0;

        for (int i = 0; i < length; i++) {
            char currentChar = input.charAt(i);

            if (Character.isUpperCase(currentChar)) {
                uppercaseCount++;
            } else if (Character.isLowerCase(currentChar)) {
                lowercaseCount++;
            } else if (Character.isDigit(currentChar)) {
                digitCount++;
            } else if (Character.isWhitespace(currentChar)) {
                whitespaceCount++;
            }
        }

        return "Length: " + length +
               ", Uppercase: " + uppercaseCount +
               ", Lowercase: " + lowercaseCount +
               ", Digits: " + digitCount +
               ", Whitespace: " + whitespaceCount;
    }

    public static String readUserInput() {
        try (BufferedReader reader = new BufferedReader(new InputStreamReader(System.in))) {
            String input = reader.readLine();
            return input == null ? "" : input;
        } catch (IOException exception) {
            return "";
        }
    }

    public static void main(String[] args) {
        String input = readUserInput();
        String result = processString(input);
        System.out.println(result);
    }
}