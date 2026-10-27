import java.util.ArrayList;
import java.util.List;
import java.util.Scanner;

public class StringConcatenator {

    public static String concatenate(List<String> strings) {
        StringBuilder sb = new StringBuilder();
        for (String str : strings) {
            sb.append(str);
        }
        return sb.toString();
    }

    public static void main(String[] args) {
        Scanner scanner = new Scanner(System.in);

        System.out.print("Enter the number of strings to concatenate: ");
        if (!scanner.hasNextInt()) {
            scanner.close();
            return;
        }

        int count = scanner.nextInt();
        scanner.nextLine();

        List<String> strings = new ArrayList<>();
        for (int i = 0; i < count; i++) {
            System.out.print("Enter string " + (i + 1) + ": ");
            strings.add(scanner.nextLine());
        }

        String result = concatenate(strings);
        System.out.println("Concatenated result: " + result);

        scanner.close();
    }
}