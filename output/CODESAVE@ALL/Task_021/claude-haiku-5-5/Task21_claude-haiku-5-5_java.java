import java.util.Arrays;

public class Main {
    public static int[] removeSmallest(int[] values) {
        if (values == null || values.length == 0) {
            return new int[0];
        }
        int minIndex = 0;
        for (int i = 1; i < values.length; i++) {
            if (values[i] < values[minIndex]) {
                minIndex = i;
            }
        }
        int[] result = new int[values.length - 1];
        for (int i = 0, j = 0; i < values.length; i++) {
            if (i != minIndex) {
                result[j++] = values[i];
            }
        }
        return result;
    }

    public static void main(String[] args) {
        System.out.println(Arrays.toString(removeSmallest(new int[]{1, 2, 3, 4, 5})));
        System.out.println(Arrays.toString(removeSmallest(new int[]{5, 3, 2, 1, 4})));
        System.out.println(Arrays.toString(removeSmallest(new int[]{2, 2, 1, 2, 1})));
        System.out.println(Arrays.toString(removeSmallest(new int[]{})));
    }
}