import java.util.Arrays;

public class MaxSubarray {

    public static class Result {
        public final int maxSum;
        public final int startIndex;
        public final int endIndex;
        public final int[] subarray;

        public Result(int maxSum, int startIndex, int endIndex, int[] subarray) {
            this.maxSum = maxSum;
            this.startIndex = startIndex;
            this.endIndex = endIndex;
            this.subarray = subarray;
        }

        @Override
        public String toString() {
            return "Maximum Sum: " + maxSum +
                   ", Indices: [" + startIndex + ", " + endIndex + "]" +
                   ", Subarray: " + Arrays.toString(subarray);
        }
    }

    public static Result findMaxSubarray(int[] nums) {
        if (nums == null || nums.length == 0) {
            throw new IllegalArgumentException("Input array must not be null or empty.");
        }

        int maxSum = nums[0];
        int currentSum = nums[0];
        int start = 0;
        int end = 0;
        int tempStart = 0;

        for (int i = 1; i < nums.length; i++) {
            if (nums[i] > currentSum + nums[i]) {
                currentSum = nums[i];
                tempStart = i;
            } else {
                currentSum += nums[i];
            }

            if (currentSum > maxSum) {
                maxSum = currentSum;
                start = tempStart;
                end = i;
            }
        }

        int[] subarray = Arrays.copyOfRange(nums, start, end + 1);
        return new Result(maxSum, start, end, subarray);
    }

    public static void main(String[] args) {
        int[] nums = {-2, 1, -3, 4, -1, 2, 1, -5, 4};
        Result result = findMaxSubarray(nums);
        System.out.println(result);
    }
}