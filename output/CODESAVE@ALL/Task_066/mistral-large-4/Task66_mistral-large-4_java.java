import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.Scanner;

public class BashExecutor {

    public static boolean validateCommand(String command) {
        if (command == null || command.trim().isEmpty()) {
            return false;
        }
        if (command.contains("\0")) {
            return false;
        }
        String lower = command.toLowerCase().trim();
        if (lower.equals("rm -rf /") || lower.equals("rm -rf /*") ||
            lower.contains(":(){ :|:& };:") || lower.startsWith("mkfs") ||
            lower.contains("dd if=/dev/zero") || lower.contains("> /dev/sda")) {
            return false;
        }
        return true;
    }

    public static String executeCommand(String command) throws IOException, InterruptedException {
        ProcessBuilder processBuilder = new ProcessBuilder("bash", "-c", command);
        processBuilder.redirectErrorStream(true);
        Process process = processBuilder.start();

        StringBuilder output = new StringBuilder();
        try (BufferedReader reader = new BufferedReader(new InputStreamReader(process.getInputStream()))) {
            String line;
            while ((line = reader.readLine()) != null) {
                output.append(line).append(System.lineSeparator());
            }
        }

        int exitCode = process.waitFor();
        if (exitCode != 0) {
            output.append("Process exited with code: ").append(exitCode);
        }

        return output.toString();
    }

    public static void main(String[] args) {
        Scanner scanner = new Scanner(System.in);
        System.out.print("Enter bash command: ");
        String command = scanner.nextLine();
        scanner.close();

        if (!validateCommand(command)) {
            System.out.println("Error: Invalid command format or unsafe content detected");
            return;
        }

        try {
            String result = executeCommand(command);
            System.out.println("Execution result:");
            System.out.print(result);
        } catch (IOException e) {
            System.out.println("Error: Failed to execute command - " + e.getMessage());
        } catch (InterruptedException e) {
            System.out.println("Error: Execution interrupted - " + e.getMessage());
            Thread.currentThread().interrupt();
        }
    }
}