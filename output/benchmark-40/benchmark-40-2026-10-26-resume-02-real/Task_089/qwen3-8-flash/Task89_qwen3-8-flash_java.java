import java.io.*;
import java.util.*;

public class Main {
    public static void main(String[] args) {
        String filePath = args.length > 0 ? args[0] : "password.txt";
        Scanner scanner = new Scanner(System.in);

        try {
            String storedPassword = readStoredPassword(filePath);
            String enteredPassword = readEnteredPassword("Enter password: ", scanner);
            boolean matches = comparePasswords(storedPassword, enteredPassword);

            if (matches) {
                System.out.println("Password matches.");
            } else {
                System.out.println("Password does not match.");
            }
        } catch (IOException e) {
            System.out.println("Error reading password file: " + e.getMessage());
        } finally {
            scanner.close();
        }
    }

    static String readStoredPassword(String filePath) throws IOException {
        try (BufferedReader reader = new BufferedReader(new FileReader(filePath))) {
            String password = reader.readLine();
            return password == null ? "" : password;
        }
    }

    static String readEnteredPassword(String prompt, Scanner scanner) {
        System.out.print(prompt);
        if (scanner.hasNextLine()) {
            return scanner.nextLine();
        }
        return "";
    }

    static boolean comparePasswords(String storedPassword, String enteredPassword) {
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
}