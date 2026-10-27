import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.regex.Pattern;

public class NumberDotMatcher {

    public static List<String> findMatchingLines(String filePath) throws IOException {
        List<String> matches = new ArrayList<>();
        Pattern pattern = Pattern.compile("^\\d+.*\\.$");

        try (BufferedReader reader = new BufferedReader(new FileReader(filePath))) {
            String line;
            while ((line = reader.readLine()) != null) {
                if (pattern.matcher(line).matches()) {
                    matches.add(line);
                }
            }
        }
        return matches;
    }

    public static boolean isMatchingLine(String line) {
        Pattern pattern = Pattern.compile("^\\d+.*\\.$");
        return pattern.matcher(line).matches();
    }

    public static void main(String[] args) {
        if (args.length < 1) {
            System.err.println("Usage: java NumberDotMatcher <file-path>");
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