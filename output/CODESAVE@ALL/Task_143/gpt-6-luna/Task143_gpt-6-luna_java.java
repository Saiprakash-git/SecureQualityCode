import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.util.Arrays;

public class Main {
    public static int[] sortArray(int[] numbers) {
        int[] sorted = numbers.clone();
        Arrays.sort(sorted);
        return sorted;
    }

    public static void main(String[] args) throws Exception {
        BufferedReader reader = new BufferedReader(new InputStreamReader(System.in));
        StringBuilder input = new StringBuilder();
        String line;

        while ((line = reader.readLine()) != null) {
            input.append(line).append(' ');
        }

        String trimmed = input.toString().trim();
        if (trimmed.isEmpty()) {
            return;
        }

        String[] tokens = trimmed.split("\\s+");
        int start = 0;
        int first = Integer.parseInt(tokens[0]);

        if (first == tokens.length - 1) {
            start = 1;
        }

        int[] numbers = new int[tokens.length - start];
        for (int i = start; i < tokens.length; i++) {
            numbers[i - start] = Integer.parseInt(tokens[i]);
        }

        int[] sorted = sortArray(numbers);
        StringBuilder output = new StringBuilder();

        for (int i = 0; i < sorted.length; i++) {
            if (i > 0) {
                output.append(' ');
            }
            output.append(sorted[i]);
        }

        System.out.println(output);
    }
}