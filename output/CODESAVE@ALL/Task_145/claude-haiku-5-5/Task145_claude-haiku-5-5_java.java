public class MaxSubarray {

    public static long[] maxSubarray(int[] nums) {
        if (nums == null || nums.length == 0) {
            return new long[]{0, -1, -1};
        }
        long best = nums[0];
        long current = nums[0];
        int bestStart = 0, bestEnd = 0, currentStart = 0;

        for (int i = 1; i < nums.length; i++) {
            if (current < 0) {
                current = nums[i];
                currentStart = i;
            } else {
                current += nums[i];
            }
            if (current > best) {
                best = current;
                bestStart = currentStart;
                bestEnd = i;
            }
        }
        return new long[]{best, bestStart, bestEnd};
    }

    public static void main(String[] args) {
        int[] input;
        if (args.length == 0) {
            input = new int[]{-2, 1, -3, 4, -1, 2, 1, -5, 4};
        } else {
            input = new int[args.length];
            for (int i = 0; i < args.length; i++) {
                input[i] = Integer.parseInt(args[i]);
            }
        }
        long[] result = maxSubarray(input);
        System.out.println("Maximum sum: " + result[0]);
        if (input.length > 0) {
            System.out.print("Subarray: [");
            for (int i = (int) result[1]; i <= (int) result[2]; i++) {
                System.out.print(input[i]);
                if (i < (int) result[2]) {
                    System.out.print(", ");
                }
            }
            System.out.println("]");
        }
    }
}