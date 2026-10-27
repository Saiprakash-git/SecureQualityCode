import java.util.Scanner;

public class FixedSizeBufferHandler {
    private static final int BUFFER_SIZE = 16;

    public static void main(String[] args) {
        Scanner scanner = new Scanner(System.in);
        char[] buffer = new char[BUFFER_SIZE];

        System.out.print("Enter text (max " + BUFFER_SIZE + " chars): ");
        String input = scanner.nextLine();

        int stored = writeToBuffer(buffer, BUFFER_SIZE, input);
        System.out.println("Characters stored: " + stored);
        System.out.println("Buffer content: [" + readFromBuffer(buffer, stored) + "]");
        System.out.println("Buffer full: " + isBufferFull(buffer, BUFFER_SIZE, stored));

        scanner.close();
    }

    public static int writeToBuffer(char[] buffer, int capacity, String input) {
        if (buffer == null || capacity <= 0 || input == null) {
            return 0;
        }

        int limit = Math.min(input.length(), capacity);
        for (int i = 0; i < limit; i++) {
            buffer[i] = input.charAt(i);
        }
        return limit;
    }

    public static String readFromBuffer(char[] buffer, int length) {
        if (buffer == null || length <= 0) {
            return "";
        }
        int validLength = Math.min(length, buffer.length);
        return new String(buffer, 0, validLength);
    }

    public static boolean isBufferFull(char[] buffer, int capacity, int used) {
        if (buffer == null || capacity <= 0) {
            return false;
        }
        return used >= capacity;
    }

    public static void clearBuffer(char[] buffer, int capacity) {
        if (buffer == null || capacity <= 0) {
            return;
        }
        int limit = Math.min(capacity, buffer.length);
        for (int i = 0; i < limit; i++) {
            buffer[i] = '\0';
        }
    }
}