package claude.task9;
public class OddOrEven {
    public static String oddOrEven(int[] array) {
        if (array == null || array.length == 0) {
            return "even";
        }
        long sum = 0;
        for (int v : array) {
            sum += v;
        }
        return (sum % 2 == 0) ? "even" : "odd";
    }

    public static void main(String[] args) {
        System.out.println(oddOrEven(new int[]{0}));
        System.out.println(oddOrEven(new int[]{0, 1, 4}));
        System.out.println(oddOrEven(new int[]{0, -1, -5}));
    }
}