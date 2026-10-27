import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.util.Scanner;

public class PasswordChecker {

    public static String readStoredPassword(String filePath) throws IOException {
        try (BufferedReader reader = new BufferedReader(new FileReader(filePath))) {
            String line = reader.readLine();
            return line == null ? "" : line;
        }
    }

    public static String readUserPassword(Scanner scanner, String prompt) {
        System.out.print(prompt);
        return scanner.hasNextLine() ? scanner.nextLine() : "";
    }

    public static boolean comparePasswords(String storedPassword, String enteredPassword) {
        if (storedPassword == null || enteredPassword == null) {
            return false;
        }
        if (storedPassword.length() != enteredPassword.length()) {
            return false;
        }
        for (int i = 0; i < storedPassword.length(); i++) {
            if (storedPassword.charAt(i) != enteredPassword.charAt(i)) {
                return false;
            }
        }
        return true;
    }

    public static String checkResultMessage(boolean matches) {
        return matches ? "Password is correct." : "Password is incorrect.";
    }

    public static void main(String[] args) {
        String filePath = args.length > 0 ? args[0] : "password.txt";
        Scanner scanner = new Scanner(System.in);

        try {
            String storedPassword = readStoredPassword(filePath);
            String enteredPassword = readUserPassword(scanner, "Enter your password: ");
            boolean matches = comparePasswords(storedPassword, enteredPassword);
            System.out.println(checkResultMessage(matches));
        } catch (IOException e) {
            System.out.println("Error reading file '" + filePath + "': " + e.getMessage());
        } finally {
            scanner.close();
        }
    }
}