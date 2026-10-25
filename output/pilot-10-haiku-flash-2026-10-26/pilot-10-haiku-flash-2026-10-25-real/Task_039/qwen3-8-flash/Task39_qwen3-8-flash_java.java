import java.io.IOException;
import java.util.Scanner;

public class Main {
    public static void main(String[] args) {
        Scanner scanner = new Scanner(System.in);

        System.out.print("Enter filename: ");
        if (!scanner.hasNextLine()) {
            scanner.close();
            return;
        }

        String filename = scanner.nextLine().trim();
        scanner.close();

        if (filename.isEmpty()) {
            return;
        }

        String osName = System.getProperty("os.name").toLowerCase();
        ProcessBuilder processBuilder;

        if (osName.contains("win")) {
            processBuilder = new ProcessBuilder("cmd", "/c", "type", filename);
        } else {
            processBuilder = new ProcessBuilder("cat", filename);
        }

        processBuilder.inheritIO();

        try {
            Process process = processBuilder.start();
            process.waitFor();
        } catch (IOException e) {
            System.err.println("Error running system command: " + e.getMessage());
        } catch (InterruptedException e) {
            System.err.println("Command was interrupted.");
            Thread.currentThread().interrupt();
        }
    }
}