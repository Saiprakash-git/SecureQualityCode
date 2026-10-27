package claude.task18;
public class BinaryConverter {
    public static int binaryArrayToInt(int[] binaryArray) {
        int result = 0;
        for (int bit : binaryArray) {
            result = (result << 1) | bit;
        }
        return result;
    }
}