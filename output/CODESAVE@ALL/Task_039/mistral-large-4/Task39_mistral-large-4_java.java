import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.util.Scanner;

public class FileContentDisplayer {
    public static String displayFileUsingSystemCommand(String filename) {
        StringBuilder result = new StringBuilder();
        try {
            String os = System.getProperty("os.name").toLowerCase();
            ProcessBuilder pb;

            if (os.contains("win")) {
                pb = new ProcessBuilder("cmd", "/c", "type", filename);
            } else {
                pb = new ProcessBuilder("cat", filename);
            }

            pb.redirectErrorStream(true);
            Process process = pb.start();

            BufferedReader reader = new BufferedReader(new InputStreamReader(process.getInputStream()));
            String line;
            while ((line = reader.readLine()) != null) {
                result.append(line).append(System.lineSeparator());
            }

            int exitCode = process.waitFor();
            if (exitCode != 0) {
                result.append("Command exited with code: ").append(exitCode);
            }

        } catch (Exception e) {
            return "Error executing command: " + e.getMessage();
        }
        return result.toString();
    }

    public static void main(String[] args) {
        Scanner scanner = new Scanner(System.in);
        System.out.print("Enter filename: ");
        String filename = scanner.nextLine();
        scanner.close();

        String contents = displayFileUsingSystemCommand(filename);
        System.out.print(contents);
    }
}