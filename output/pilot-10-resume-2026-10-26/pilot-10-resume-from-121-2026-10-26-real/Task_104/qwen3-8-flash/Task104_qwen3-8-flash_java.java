import java.io.IOException;
import java.io.InputStreamReader;
import java.io.Reader;
import java.nio.charset.StandardCharsets;

public class Main {
    private static final int DEFAULT_BUFFER_SIZE = 64;

    public static void main(String[] args) throws IOException {
        String input = readIntoFixedBuffer(
            new InputStreamReader(System.in, StandardCharsets.UTF_8),
            DEFAULT_BUFFER_SIZE
        );
        System.out.println(input);
    }

    public static String readIntoFixedBuffer(Reader reader, int bufferSize) throws IOException {
        if (reader == null) {
            throw new IllegalArgumentException("reader must not be null");
        }
        if (bufferSize <= 0) {
            throw new IllegalArgumentException("bufferSize must be positive");
        }

        char[] buffer = new char[bufferSize];
        int length = 0;
        int ch;

        while (length < bufferSize && (ch = reader.read()) != -1) {
            if (ch == '\n') {
                if (length > 0 && buffer[length - 1] == '\r') {
                    length--;
                }
                break;
            }
            buffer[length++] = (char) ch;
        }

        if (length == bufferSize) {
            while ((ch = reader.read()) != -1 && ch != '\n') {
            }
            if (ch == '\n' && length > 0 && buffer[length - 1] == '\r') {
                length--;
            }
        }

        return new String(buffer, 0, length);
    }
}