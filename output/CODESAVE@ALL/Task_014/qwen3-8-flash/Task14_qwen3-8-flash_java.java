class Solution {
    public static int findEvenIndex(int[] arr) {
        if (arr == null || arr.length == 0) {
            return -1;
        }

        long total = 0;
        for (int value : arr) {
            total += value;
        }

        long leftSum = 0;
        for (int i = 0; i < arr.length; i++) {
            long rightSum = total - leftSum - arr[i];
            if (leftSum == rightSum) {
                return i;
            }
            leftSum += arr[i];
        }

        return -1;
    }
}