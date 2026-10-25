import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.Scanner;

public class FileDisplay {

    public static String displayFileContents(String filename) throws IOException, InterruptedException {
        boolean isWindows = System.getProperty("os.name").toLowerCase().startsWith("windows");
        ProcessBuilder processBuilder;

        if (isWindows) {
            processBuilder = new ProcessBuilder("cmd.exe", "/c", "type", filename);
        } else {
            processBuilder = new ProcessBuilder("cat", filename);
        }

        Process process = processBuilder.start();
        StringBuilder output = new StringBuilder();

        try (BufferedReader reader = new BufferedReader(new InputStreamReader(process.getInputStream()))) {
            String line;
            while ((line = reader.readLine()) != null) {
                output.append(line).append(System.lineSeparator());
            }
        }

        try (BufferedReader errorReader = new BufferedReader(new InputStreamReader(process.getErrorStream()))) {
            String line;
            while ((line = errorReader.readLine()) != null) {
                output.append(line).append(System.lineSeparator());
            }
        }

        process.waitFor();
        return output.toString();
    }

    public static void main(String[] args) {
        Scanner scanner = new Scanner(System.in);
        System.out.print("Enter filename: ");
        if (scanner.hasNextLine()) {
            String filename = scanner.nextLine().trim();
            try {
                String contents = displayFileContents(filename);
                System.out.print(contents);
            } catch (IOException | InterruptedException e) {
                System.err.println("Error executing command: " + e.getMessage());
            }
        }
    }
}