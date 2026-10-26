import java.io.BufferedReader;
import java.io.InputStreamReader;

public class Main {
    public static boolean isPangram(String input) {
        if (input == null) {
            return false;
        }

        int mask = 0;

        for (int i = 0; i < input.length(); i++) {
            char c = input.charAt(i);

            if (c >= 'a' && c <= 'z') {
                mask |= 1 << (c - 'a');
            } else if (c >= 'A' && c <= 'Z') {
                mask |= 1 << (c - 'A');
            }
        }

        return mask == (1 << 26) - 1;
    }

    public static void main(String[] args) throws Exception {
        BufferedReader reader = new BufferedReader(new InputStreamReader(System.in));
        String input = reader.readLine();
        System.out.println(isPangram(input));
    }
}