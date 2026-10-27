import java.io.BufferedReader;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.regex.Pattern;

public class Main {
    private static final Pattern LINE_PATTERN = Pattern.compile("^\\d+.*\\.$");

    public static void main(String[] args) throws IOException {
        if (args.length != 1) {
            throw new IllegalArgumentException("Usage: java Main <file>");
        }

        try (BufferedReader reader = Files.newBufferedReader(Path.of(args[0]))) {
            String line;
            while ((line = reader.readLine()) != null) {
                if (LINE_PATTERN.matcher(line).matches()) {
                    System.out.println(line);
                }
            }
        }
    }
}