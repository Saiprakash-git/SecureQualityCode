import java.io.BufferedReader;
import java.io.InputStreamReader;

public class Main {
    public static boolean isValidAdminPassword(String password) {
        if (password == null || password.length() < 8) {
            return false;
        }

        boolean hasLower = false;
        boolean hasUpper = false;
        boolean hasDigit = false;
        boolean hasSpecial = false;

        for (int i = 0; i < password.length(); i++) {
            char c = password.charAt(i);

            if (Character.isWhitespace(c)) {
                return false;
            }

            if (Character.isLowerCase(c)) {
                hasLower = true;
            } else if (Character.isUpperCase(c)) {
                hasUpper = true;
            } else if (Character.isDigit(c)) {
                hasDigit = true;
            } else if (!Character.isLetterOrDigit(c)) {
                hasSpecial = true;
            }
        }

        return hasLower && hasUpper && hasDigit && hasSpecial;
    }

    public static void main(String[] args) throws Exception {
        BufferedReader reader = new BufferedReader(new InputStreamReader(System.in));
        String password = reader.readLine();

        if (isValidAdminPassword(password)) {
            System.out.println("Valid");
        } else {
            System.out.println("Invalid");
        }
    }
}