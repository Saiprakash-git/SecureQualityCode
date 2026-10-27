package claude.task89;
import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.util.Scanner;

public class PasswordComparator {

    public static String readPasswordFromFile(String filename) throws IOException {
        BufferedReader reader = new BufferedReader(new FileReader(filename));
        String password = reader.readLine();
        reader.close();
        return password;
    }

    public static boolean comparePasswordsSequentially(String stored, String entered) {
        if (stored == null || entered == null) {
            return false;
        }
        if (stored.length() != entered.length()) {
            return false;
        }
        for (int i = 0; i < stored.length(); i++) {
            if (stored.charAt(i) != entered.charAt(i)) {
                return false;
            }
        }
        return true;
    }

    public static String getUserInput(Scanner scanner, String prompt) {
        System.out.print(prompt);
        return scanner.nextLine();
    }

    public static void main(String[] args) {
        String filename = "password.txt";
        Scanner scanner = new Scanner(System.in);

        try {
            String storedPassword = readPasswordFromFile(filename);
            String userPassword = getUserInput(scanner, "Enter password: ");

            boolean isMatch = comparePasswordsSequentially(storedPassword, userPassword);

            if (isMatch) {
                System.out.println("Passwords match.");
            } else {
                System.out.println("Passwords do not match.");
            }
        } catch (IOException e) {
            System.out.println("Error reading file: " + e.getMessage());
        } finally {
            scanner.close();
        }
    }
}