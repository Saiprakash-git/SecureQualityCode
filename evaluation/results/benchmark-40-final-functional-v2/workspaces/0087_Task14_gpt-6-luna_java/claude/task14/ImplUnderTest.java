package claude.task14;
class Solution {
    public static int findEvenIndex(int[] arr) {
        long total = 0;
        for (int value : arr) {
            total += value;
        }

        long leftSum = 0;
        for (int i = 0; i < arr.length; i++) {
            if (leftSum == total - leftSum - arr[i]) {
                return i;
            }
            leftSum += arr[i];
        }

        return -1;
    }
}