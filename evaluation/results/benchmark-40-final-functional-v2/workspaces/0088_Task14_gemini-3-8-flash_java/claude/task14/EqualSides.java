package claude.task14;
public class EqualSides {
    public static int findEvenIndex(int[] arr) {
        if (arr == null || arr.length == 0) {
            return -1;
        }

        long totalSum = 0;
        for (int num : arr) {
            totalSum += num;
        }

        long leftSum = 0;
        for (int i = 0; i < arr.length; i++) {
            if (leftSum == totalSum - leftSum - arr[i]) {
                return i;
            }
            leftSum += arr[i];
        }

        return -1;
    }
}