```java
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;
import java.util.regex.Pattern;

public class NumberDotLineMatcher {

    private static final Pattern PATTERN = Pattern.compile("^\\d+.*\\.$");

    public static List<String> findMatchingLines(String filePath) throws IOException {
        return findMatchingLines(Paths.get(filePath));
    }

    public static List<String> findMatchingLines(Path path) throws IOException {
        List<String> matches = new ArrayList<>();
        List<String> lines = Files.readAllLines(path, StandardCharsets.UTF_8);
        for (String line : lines) {
            if (PATTERN.matcher(line).matches()) {
                matches.add(line);
            }
        }
        return matches;
    }

    public static void main(String[] args) {
        if (args.length < 1) {
            System.err.println("Usage: java NumberDotLineMatcher <file>");
            System.exit(1);
        }
        try {
            List<String> matches = findMatchingLines(args[0]);
            for (String line : matches) {
                System.out.println(line);
            }
        } catch (IOException e) {
            System.err.println("Error reading file: " + e.getMessage());
            System.exit(1);
        }
    }
}
```