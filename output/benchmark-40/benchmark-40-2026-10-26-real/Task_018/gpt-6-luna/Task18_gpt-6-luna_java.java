import java.util.List;

public class BinaryArrayToNumber {
    public static int ConvertBinaryArrayToInt(List<Integer> binary) {
        int value = 0;
        for (int bit : binary) {
            value = (value << 1) | bit;
        }
        return value;
    }

    public static int binaryArrayToNumber(int[] binary) {
        int value = 0;
        for (int bit : binary) {
            value = (value << 1) | bit;
        }
        return value;
    }
}