import java.util.Arrays;

public class ArraySorter {

    public static int[] sortAscending(int[] array) {
        if (array == null) {
            return null;
        }
        int[] sorted = array.clone();
        Arrays.sort(sorted);
        return sorted;
    }

    public static void main(String[] args) {
        int[] numbers = {64, 34, 25, 12, 22, 11, 90};
        int[] sortedNumbers = sortAscending(numbers);
        System.out.println(Arrays.toString(sortedNumbers));
    }
}