package claude.task18;
import java.util.Arrays;
import java.util.Scanner;

public class BinaryArrayToInteger {

    public static int binaryArrayToInt(int[] bits) {
        int result = 0;
        for (int bit : bits) {
            result = (result << 1) | (bit & 1);
        }
        return result;
    }

    public static void main(String[] args) {
        if (args.length > 0) {
            int[] bits = new int[args.length];
            for (int i = 0; i < args.length; i++) {
                bits[i] = Integer.parseInt(args[i].trim());
            }
            System.out.println(binaryArrayToInt(bits));
        } else {
            Scanner scanner = new Scanner(System.in);
            while (scanner.hasNextLine()) {
                String line = scanner.nextLine().trim();
                if (line.isEmpty()) {
                    continue;
                }
                String cleaned = line.replaceAll("[\\[\\]\\s]", "");
                String[] parts = cleaned.split(",");
                int[] bits = new int[parts.length];
                for (int i = 0; i < parts.length; i++) {
                    bits[i] = Integer.parseInt(parts[i]);
                }
                System.out.println(binaryArrayToInt(bits));
            }
            scanner.close();
        }
    }
}