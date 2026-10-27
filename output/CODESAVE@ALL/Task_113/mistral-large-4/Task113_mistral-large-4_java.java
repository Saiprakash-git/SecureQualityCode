import java.util.Scanner;
import java.util.ArrayList;
import java.util.List;

public class StringConcatenator {

    public static String concatenate(String[] strings) {
        StringBuilder sb = new StringBuilder();
        for (String s : strings) {
            sb.append(s);
        }
        return sb.toString();
    }

    public static String concatenate(List<String> strings) {
        StringBuilder sb = new StringBuilder();
        for (String s : strings) {
            sb.append(s);
        }
        return sb.toString();
    }

    public static void main(String[] args) {
        Scanner scanner = new Scanner(System.in);
        List<String> strings = new ArrayList<>();

        System.out.println("Enter strings to concatenate (type 'done' on a new line to finish):");
        while (scanner.hasNextLine()) {
            String input = scanner.nextLine();
            if (input.equalsIgnoreCase("done")) {
                break;
            }
            strings.add(input);
        }

        String result = concatenate(strings);
        System.out.println("Concatenated result: " + result);
        scanner.close();
    }
}