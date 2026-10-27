import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.util.Scanner;

public class PasswordChecker {

    public static String readPasswordFromFile(String filePath) throws IOException {
        try (BufferedReader reader = new BufferedReader(new FileReader(filePath))) {
            String line = reader.readLine();
            return (line != null) ? line.trim() : "";
        }
    }

    public static boolean comparePasswordsSequentially(String storedPassword, String enteredPassword) {
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

    public static boolean verifyUserPassword(String filePath, String enteredPassword) {
        try {
            String storedPassword = readPasswordFromFile(filePath);
            return comparePasswordsSequentially(storedPassword, enteredPassword);
        } catch (IOException e) {
            System.err.println("Error reading password file: " + e.getMessage());
            return false;
        }
    }

    public static void main(String[] args) {
        Scanner scanner = new Scanner(System.in);

        System.out.print("Enter path to password file: ");
        String filePath = scanner.nextLine();

        System.out.print("Enter password: ");
        String enteredPassword = scanner.nextLine();

        boolean isMatch = verifyUserPassword(filePath, enteredPassword);

        if (isMatch) {
            System.out.println("Password verification successful: Access granted.");
        } else {
            System.out.println("Password verification failed: Access denied.");
        }

        scanner.close();
    }
}