import java.io.BufferedReader;
import java.io.Console;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Scanner;

public class Main {
    public static void main(String[] args) {
        Path passwordFile = Path.of(args.length > 0 ? args[0] : "password.txt");

        try {
            String storedPassword = readStoredPassword(passwordFile);
            String enteredPassword = readEnteredPassword();

            System.out.println(passwordsMatch(storedPassword, enteredPassword)
                    ? "Password matches."
                    : "Password does not match.");
        } catch (IOException e) {
            System.err.println("Unable to read the password file: " + e.getMessage());
        }
    }

    private static String readStoredPassword(Path file) throws IOException {
        try (BufferedReader reader = Files.newBufferedReader(file, StandardCharsets.UTF_8)) {
            String password = reader.readLine();
            if (password == null) {
                throw new IOException("The password file is empty.");
            }
            return password;
        }
    }

    private static String readEnteredPassword() {
        Console console = System.console();
        if (console != null) {
            char[] password = console.readPassword("Enter password: ");
            return password == null ? "" : new String(password);
        }

        System.out.print("Enter password: ");
        try (Scanner scanner = new Scanner(System.in)) {
            return scanner.nextLine();
        }
    }

    private static boolean passwordsMatch(String storedPassword, String enteredPassword) {
        if (storedPassword == null || enteredPassword == null) {
            return false;
        }

        boolean matches = storedPassword.length() == enteredPassword.length();
        int length = Math.min(storedPassword.length(), enteredPassword.length());

        for (int i = 0; i < length; i++) {
            if (storedPassword.charAt(i) != enteredPassword.charAt(i)) {
                matches = false;
            }
        }

        return matches;
    }
}