import java.io.*;
import java.util.*;

public class FileViewer {

    static List<String> buildCommand(String filename, String osName) {
        if (osName.toLowerCase().contains("win")) {
            return Arrays.asList("cmd", "/c", "type", filename);
        }
        return Arrays.asList("cat", filename);
    }

    static String runCommand(List<String> command) throws IOException, InterruptedException {
        ProcessBuilder pb = new ProcessBuilder(command);
        pb.redirectErrorStream(true);
        Process process = pb.start();

        StringBuilder output = new StringBuilder();
        try (BufferedReader reader = new BufferedReader(
                new InputStreamReader(process.getInputStream()))) {
            String line;
            while ((line = reader.readLine()) != null) {
                output.append(line).append(System.lineSeparator());
            }
        }

        process.waitFor();
        return output.toString();
    }

    static String displayFile(String filename) throws IOException, InterruptedException {
        if (filename == null || filename.trim().isEmpty()) {
            return "Error: no filename provided." + System.lineSeparator();
        }
        List<String> command = buildCommand(filename.trim(), System.getProperty("os.name"));
        return runCommand(command);
    }

    public static void main(String[] args) {
        Scanner scanner = new Scanner(System.in);
        System.out.print("Enter filename: ");
        String filename = scanner.nextLine();

        try {
            String contents = displayFile(filename);
            System.out.print(contents);
        } catch (IOException | InterruptedException e) {
            System.err.println("Error running system command: " + e.getMessage());
            if (e instanceof InterruptedException) {
                Thread.currentThread().interrupt();
            }
        } finally {
            scanner.close();
        }
    }
}