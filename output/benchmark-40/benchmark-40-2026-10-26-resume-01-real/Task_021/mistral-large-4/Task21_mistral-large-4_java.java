public class Museum {
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
}