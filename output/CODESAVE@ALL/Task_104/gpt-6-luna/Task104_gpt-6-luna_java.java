import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;

public class Main {
    private static final int BUFFER_SIZE = 100;

    public static void main(String[] args) throws IOException {
        char[] buffer = new char[BUFFER_SIZE];
        String input = readInput(new BufferedReader(new InputStreamReader(System.in)));
        int length = storeInput(input, buffer);

        System.out.println(new String(buffer, 0, length));
        if (input.length() > BUFFER_SIZE) {
            System.err.println("Input exceeded the " + BUFFER_SIZE + "-character limit and was truncated.");
        }
    }

    private static String readInput(BufferedReader reader) throws IOException {
        String input = reader.readLine();
        return input == null ? "" : input;
    }

    private static int storeInput(String input, char[] buffer) {
        int length = Math.min(input.length(), buffer.length);
        input.getChars(0, length, buffer, 0);
        return length;
    }
}