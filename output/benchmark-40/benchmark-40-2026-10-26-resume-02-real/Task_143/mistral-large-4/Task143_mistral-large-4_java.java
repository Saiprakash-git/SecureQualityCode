import java.util.Arrays;

public class IntegerArraySorter {
    public static int[] sortAscending(int[] numbers) {
        if (numbers == null) {
            return new int[0];
        }
        int[] sorted = Arrays.copyOf(numbers, numbers.length);
        Arrays.sort(sorted);
        return sorted;
    }

    public static void main(String[] args) {
        int[] input = {64, 34, 25, 12, 22, 11, 90};
        int[] result = sortAscending(input);
        System.out.println("Sorted array: " + Arrays.toString(result));
    }
}