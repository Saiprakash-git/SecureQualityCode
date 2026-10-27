import java.util.ArrayList;
import java.util.List;
import java.util.Scanner;

public class ConcatenateStrings {

    public static String concatenate(List<String> parts) {
        StringBuilder sb = new StringBuilder();
        for (String part : parts) {
            sb.append(part);
        }
        return sb.toString();
    }

    public static List<String> readStrings(Scanner scanner) {
        List<String> parts = new ArrayList<>();
        System.out.println("Enter strings to concatenate (type 'done' on a line by itself to finish):");
        while (scanner.hasNextLine()) {
            String line = scanner.nextLine();
            if (line.trim().equalsIgnoreCase("done")) {
                break;
            }
            parts.add(line);
        }
        return parts;
    }

    public static void main(String[] args) {
        Scanner scanner = new Scanner(System.in);
        List<String> parts = readStrings(scanner);
        String result = concatenate(parts);
        System.out.println("Concatenated result: " + result);
        scanner.close();
    }
}