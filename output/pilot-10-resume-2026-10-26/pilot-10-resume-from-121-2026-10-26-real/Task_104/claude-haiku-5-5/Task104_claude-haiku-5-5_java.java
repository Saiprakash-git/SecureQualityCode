import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;

public class Main {

    static final int BUFFER_SIZE = 16;

    static char[] createBuffer(int size) {
        if (size <= 0) {
            throw new IllegalArgumentException("Buffer size must be positive");
        }
        return new char[size];
    }

    static int storeInput(char[] buffer, String input) {
        if (buffer == null) {
            throw new IllegalArgumentException("Buffer must not be null");
        }
        String safeInput = (input == null) ? "" : input;
        int count = Math.min(buffer.length, safeInput.length());
        for (int i = 0; i < count; i++) {
            buffer[i] = safeInput.charAt(i);
        }
        for (int i = count; i < buffer.length; i++) {
            buffer[i] = '\0';
        }
        return count;
    }

    static boolean isTruncated(String input, int bufferSize) {
        return input != null && input.length() > bufferSize;
    }

    static String bufferContents(char[] buffer) {
        int length = 0;
        while (length < buffer.length && buffer[length] != '\0') {
            length++;
        }
        return new String(buffer, 0, length);
    }

    static String processInput(String input, int bufferSize) {
        char[] buffer = createBuffer(bufferSize);
        int stored = storeInput(buffer, input);
        boolean truncated = isTruncated(input, bufferSize);
        StringBuilder result = new StringBuilder();
        result.append("Stored ").append(stored).append(" of ")
              .append(input == null ? 0 : input.length())
              .append(" characters");
        if (truncated) {
            result.append(" (input truncated)");
        }
        result.append(System.lineSeparator());
        result.append("Buffer: [").append(bufferContents(buffer)).append("]");
        return result.toString();
    }

    static String readLineFromReader(BufferedReader reader) throws IOException {
        return reader.readLine();
    }

    public static void main(String[] args) {
        System.out.println("Fixed-size buffer input (size " + BUFFER_SIZE + ").");
        System.out.println("Type text and press Enter. Type 'quit' to exit.");

        try (BufferedReader reader = new BufferedReader(new InputStreamReader(System.in))) {
            String line;
            while ((line = readLineFromReader(reader)) != null) {
                if (line.equalsIgnoreCase("quit")) {
                    break;
                }
                System.out.println(processInput(line, BUFFER_SIZE));
            }
        } catch (IOException e) {
            System.err.println("Input error: " + e.getMessage());
        }
    }
}