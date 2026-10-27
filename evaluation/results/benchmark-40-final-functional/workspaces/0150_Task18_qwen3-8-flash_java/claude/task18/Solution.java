package claude.task18;
public class Solution {
    public static int binaryArrayToNumber(int[] binaryArray) {
        int result = 0;

        for (int bit : binaryArray) {
            result = result * 2 + bit;
        }

        return result;
    }
}