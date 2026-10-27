public class Solution {
    public static int[] removeSmallest(int[] numbers) {
        if (numbers == null || numbers.length == 0) {
            return new int[0];
        }

        int minIndex = 0;

        for (int i = 1; i < numbers.length; i++) {
            if (numbers[i] < numbers[minIndex]) {
                minIndex = i;
            }
        }

        int[] result = new int[numbers.length - 1];

        for (int i = 0, j = 0; i < numbers.length; i++) {
            if (i != minIndex) {
                result[j++] = numbers[i];
            }
        }

        return result;
    }
}