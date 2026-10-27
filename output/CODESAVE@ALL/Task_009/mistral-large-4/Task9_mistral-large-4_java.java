public class Solution {
    public static String oddOrEven(int[] nums) {
        if (nums == null || nums.length == 0) {
            return "even";
        }
        int oddCount = 0;
        for (int num : nums) {
            if (num % 2 != 0) {
                oddCount++;
            }
        }
        return oddCount % 2 == 1 ? "odd" : "even";
    }
}