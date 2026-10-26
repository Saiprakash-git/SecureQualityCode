import java.util.List;

public class Solution {
    public static String oddOrEven(int[] arr) {
        int parity = 0;
        if (arr != null) {
            for (int n : arr) {
                parity ^= (n & 1);
            }
        }
        return parity == 0 ? "even" : "odd";
    }

    public static String oddOrEven(List<Integer> arr) {
        int parity = 0;
        if (arr != null) {
            for (Integer n : arr) {
                if (n != null) {
                    parity ^= (n & 1);
                }
            }
        }
        return parity == 0 ? "even" : "odd";
    }
}