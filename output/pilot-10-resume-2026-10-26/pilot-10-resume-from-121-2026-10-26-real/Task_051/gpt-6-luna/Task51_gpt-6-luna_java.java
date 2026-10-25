import java.io.BufferedReader;
import java.io.InputStreamReader;

public class Main {
    public static String encrypt(String text) {
        StringBuilder encrypted = new StringBuilder(text.length());

        for (int i = 0; i < text.length(); i++) {
            char c = text.charAt(i);

            if (c >= 'a' && c <= 'z') {
                encrypted.append((char) ('a' + (c - 'a' + 3) % 26));
            } else if (c >= 'A' && c <= 'Z') {
                encrypted.append((char) ('A' + (c - 'A' + 3) % 26));
            } else {
                encrypted.append(c);
            }
        }

        return encrypted.toString();
    }

    public static void main(String[] args) throws Exception {
        BufferedReader reader = new BufferedReader(new InputStreamReader(System.in));
        String text = reader.readLine();

        if (text != null) {
            System.out.println(encrypt(text));
        }
    }
}