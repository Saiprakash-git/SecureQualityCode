public class Solution {
    public static String oddOrEven(int[] array) {
        if (array == null || array.length == 0) {
            return "even";
        }
        int sum = 0;
        for (int n : array) {
            sum += n;
        }
        return (sum % 2 != 0) ? "odd" : "even";
    }
}