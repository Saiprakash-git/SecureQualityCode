import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.regex.Pattern;

public class Main {
    private static final Pattern STARTS_WITH_NUMBER_AND_ENDS_WITH_DOT =
            Pattern.compile("^\\d+.*\\.$");

    public static boolean isMatchingLine(String line) {
        return line != null && STARTS_WITH_NUMBER_AND_ENDS_WITH_DOT.matcher(line).matches();
    }

    public static List<String> readMatchingLines(String filePath) throws IOException {
        List<String> matches = new ArrayList<>();

        try (BufferedReader reader = new BufferedReader(new FileReader(filePath))) {
            String line;
            while ((line = reader.readLine()) != null) {
                if (isMatchingLine(line)) {
                    matches.add(line);
                }
            }
        }

        return matches;
    }

    public static String formatMatches(List<String> matches) {
        StringBuilder output = new StringBuilder();

        for (String match : matches) {
            output.append(match).append(System.lineSeparator());
        }

        return output.toString();
    }

    public static void main(String[] args) throws IOException {
        if (args.length < 1) {
            System.out.println("Usage: java Main <file-path>");
            return;
        }

        List<String> matches = readMatchingLines(args[0]);
        System.out.print(formatMatches(matches));
    }
}