import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.regex.Pattern;

public class RegexLineMatcher {

    private static final Pattern PATTERN = Pattern.compile("^\\d+.*\\.$");

    public static List<String> findMatchingLines(String filePath) throws IOException {
        List<String> matchingLines = new ArrayList<>();
        try (BufferedReader reader = new BufferedReader(new FileReader(filePath))) {
            String line;
            while ((line = reader.readLine()) != null) {
                if (PATTERN.matcher(line).matches()) {
                    matchingLines.add(line);
                }
            }
        }
        return matchingLines;
    }

    public static void main(String[] args) {
        if (args.length == 0) {
            System.err.println("Usage: java RegexLineMatcher <file-path>");
            return;
        }

        try {
            List<String> matches = findMatchingLines(args[0]);
            for (String match : matches) {
                System.out.println(match);
            }
        } catch (IOException e) {
            System.err.println("Error reading file: " + e.getMessage());
        }
    }
}