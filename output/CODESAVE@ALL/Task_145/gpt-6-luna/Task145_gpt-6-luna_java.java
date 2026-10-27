import java.util.Scanner;

public class Main {
    public static long maxSubarraySum(int[] numbers) {
        if (numbers.length == 0) {
            return 0;
        }

        long currentSum = numbers[0];
        long maximumSum = numbers[0];

        for (int i = 1; i < numbers.length; i++) {
            currentSum = Math.max(numbers[i], currentSum + numbers[i]);
            maximumSum = Math.max(maximumSum, currentSum);
        }

        return maximumSum;
    }

    public static void main(String[] args) {
        Scanner scanner = new Scanner(System.in);

        if (!scanner.hasNextInt()) {
            return;
        }

        int size = scanner.nextInt();
        int[] numbers = new int[size];

        for (int i = 0; i < size; i++) {
            numbers[i] = scanner.nextInt();
        }

        System.out.println(maxSubarraySum(numbers));
    }
}