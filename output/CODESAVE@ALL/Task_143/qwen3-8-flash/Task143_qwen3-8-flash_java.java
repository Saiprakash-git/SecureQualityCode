import java.util.Arrays;
import java.util.Scanner;

public class Main {
    public static int[] sortAscending(int[] input) {
        if (input == null) {
            return new int[0];
        }
        int[] sorted = Arrays.copyOf(input, input.length);
        Arrays.sort(sorted);
        return sorted;
    }

    public static int[] sort(int[] input) {
        return sortAscending(input);
    }

    public static int[] sortArray(int[] input) {
        return sortAscending(input);
    }

    public static int[] parseIntegers(String input) {
        if (input == null || input.trim().isEmpty()) {
            return new int[0];
        }

        Scanner scanner = new Scanner(input);
        int[] temp = new int[16];
        int count = 0;

        while (scanner.hasNextInt()) {
            if (count == temp.length) {
                temp = Arrays.copyOf(temp, temp.length * 2);
            }
            temp[count++] = scanner.nextInt();
        }

        scanner.close();
        return Arrays.copyOf(temp, count);
    }

    public static String formatArray(int[] input) {
        if (input == null) {
            return "";
        }

        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < input.length; i++) {
            if (i > 0) {
                sb.append(' ');
            }
            sb.append(input[i]);
        }

        return sb.toString();
    }

    public static void main(String[] args) {
        String input;

        if (args.length > 0) {
            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < args.length; i++) {
                if (i > 0) {
                    sb.append(' ');
                }
                sb.append(args[i]);
            }
            input = sb.toString();
        } else {
            Scanner systemScanner = new Scanner(System.in);
            StringBuilder sb = new StringBuilder();
            while (systemScanner.hasNextLine()) {
                sb.append(systemScanner.nextLine()).append(' ');
            }
            systemScanner.close();
            input = sb.toString();
        }

        int[] numbers = parseIntegers(input);
        int[] sorted = sortAscending(numbers);
        System.out.println(formatArray(sorted));
    }
}