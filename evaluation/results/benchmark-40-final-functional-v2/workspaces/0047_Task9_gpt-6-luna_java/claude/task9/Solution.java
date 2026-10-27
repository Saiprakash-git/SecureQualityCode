package claude.task9;
public class Solution {
    public static String oddOrEven(int[] array) {
        int parity = 0;
        for (int value : array) {
            parity ^= value & 1;
        }
        return parity == 0 ? "even" : "odd";
    }
}