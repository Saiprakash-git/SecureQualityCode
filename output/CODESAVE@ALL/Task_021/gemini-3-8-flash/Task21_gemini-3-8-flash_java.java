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
        System.arraycopy(numbers, 0, result, 0, minIndex);
        System.arraycopy(numbers, minIndex + 1, result, minIndex, numbers.length - minIndex - 1);

        return result;
    }

    public static void main(String[] args) {
        int[] example1 = removeSmallest(new int[]{1, 2, 3, 4, 5});
        int[] example2 = removeSmallest(new int[]{5, 3, 2, 1, 4});
        int[] example3 = removeSmallest(new int[]{2, 2, 1, 2, 1});
    }
}