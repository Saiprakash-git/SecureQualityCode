import java.util.Arrays;

public class Kata {
    public static int[] removeSmallest(int[] numbers) {
        if (numbers == null || numbers.length == 0) {
            return new int[0];
        }

        int smallestIndex = 0;
        for (int i = 1; i < numbers.length; i++) {
            if (numbers[i] < numbers[smallestIndex]) {
                smallestIndex = i;
            }
        }

        int[] result = new int[numbers.length - 1];
        System.arraycopy(numbers, 0, result, 0, smallestIndex);
        System.arraycopy(numbers, smallestIndex + 1, result, smallestIndex,
                numbers.length - smallestIndex - 1);
        return result;
    }
}