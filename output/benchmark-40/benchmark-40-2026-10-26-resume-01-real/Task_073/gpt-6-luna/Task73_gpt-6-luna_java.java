import java.io.BufferedReader;
import java.io.InputStreamReader;

public class Main {
    public static boolean isValidAdminPassword(String password) {
        if (password == null || password.length() < 12) {
            return false;
        }

        boolean hasUppercase = false;
        boolean hasLowercase = false;
        boolean hasDigit = false;
        boolean hasSpecialCharacter = false;

        for (char c : password.toCharArray()) {
            if (Character.isWhitespace(c) || Character.isISOControl(c)) {
                return false;
            }

            if (Character.isUpperCase(c)) {
                hasUppercase = true;
            } else if (Character.isLowerCase(c)) {
                hasLowercase = true;
            } else if (Character.isDigit(c)) {
                hasDigit = true;
            } else {
                hasSpecialCharacter = true;
            }
        }

        return hasUppercase && hasLowercase && hasDigit && hasSpecialCharacter;
    }

    public static void main(String[] args) throws Exception {
        BufferedReader reader = new BufferedReader(new InputStreamReader(System.in));
        String password = reader.readLine();
        System.out.println(isValidAdminPassword(password) ? "Valid" : "Invalid");
    }
}