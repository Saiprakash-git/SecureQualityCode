import java.util.List;

public class BinaryArrayToNumber {

    public static int convertBinaryArrayToInt(List<Integer> binary) {
        int result = 0;
        for (int bit : binary) {
            result = (result << 1) | bit;
        }
        return result;
    }

    public static int convertBinaryArrayToInt(int[] binary) {
        int result = 0;
        for (int bit : binary) {
            result = (result << 1) | bit;
        }
        return result;
    }
}