import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;

public class SafeStringProcessor {

    private static final int MAX_BUFFER_SIZE = 1024;

    public static String readBoundedInput(InputStream in, int maxLength) throws IOException {
        if (in == null || maxLength <= 0) {
            return "";
        }

        BufferedReader reader = new BufferedReader(new InputStreamReader(in, StandardCharsets.UTF_8));
        char[] buffer = new char[maxLength];
        int totalRead = 0;

        while (totalRead < maxLength) {
            int read = reader.read(buffer, totalRead, maxLength - totalRead);
            if (read == -1) {
                break;
            }
            
            int previousTotal = totalRead;
            totalRead += read;

            for (int i = previousTotal; i < totalRead; i++) {
                if (buffer[i] == '\n' || buffer[i] == '\r') {
                    return new String(buffer, 0, i);
                }
            }
        }

        return new String(buffer, 0, totalRead);
    }

    public static String processString(String input) {
        if (input == null) {
            return "";
        }

        String trimmed = input.trim();
        StringBuilder processed = new StringBuilder(trimmed.length());

        for (int i = 0; i < trimmed.length(); i++) {
            char c = trimmed.charAt(i);
            if (Character.isLetterOrDigit(c) || Character.isWhitespace(c)) {
                processed.append(Character.toUpperCase(c));
            } else {
                processed.append('?');
            }
        }

        return processed.toString();
    }

    public static void main(String[] args) {
        System.out.print("Enter input (max " + MAX_BUFFER_SIZE + " characters): ");
        try {
            String userInput = readBoundedInput(System.in, MAX_BUFFER_SIZE);
            String processedOutput = processString(userInput);
            System.out.println("Result: " + processedOutput);
        } catch (IOException e) {
            System.err.println("Input processing error: " + e.getMessage());
        }
    }
}