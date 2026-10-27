import java.util.ArrayList;
import java.util.List;
import java.util.Scanner;

public class Main {
    public static String concatenate(String[] strings) {
        StringBuilder result = new StringBuilder();

        for (String str : strings) {
            if (str != null) {
                result.append(str);
            }
        }

        return result.toString();
    }

    public static void main(String[] args) {
        Scanner scanner = new Scanner(System.in);
        List<String> strings = new ArrayList<>();

        while (scanner.hasNext()) {
            strings.add(scanner.next());
        }

        scanner.close();

        String concatenated = concatenate(strings.toArray(new String[0]));
        System.out.print(concatenated);
    }
}