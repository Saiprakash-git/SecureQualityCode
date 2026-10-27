import java.io.IOException;
import java.nio.file.Paths;
import java.util.Scanner;

public class Main {
    public static int displayFile(String filename) throws IOException, InterruptedException {
        Process process = new ProcessBuilder(
                "cat",
                Paths.get(filename).toAbsolutePath().normalize().toString()
        ).inheritIO().start();

        return process.waitFor();
    }

    public static void main(String[] args) throws IOException, InterruptedException {
        System.out.print("Enter filename: ");
        try (Scanner scanner = new Scanner(System.in)) {
            String filename = scanner.nextLine();
            int exitCode = displayFile(filename);
            if (exitCode != 0) {
                System.exit(exitCode);
            }
        }
    }
}