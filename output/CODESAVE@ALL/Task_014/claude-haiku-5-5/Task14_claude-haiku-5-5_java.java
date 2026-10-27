**Output:** Java source file with a class and a method that takes the array and returns the index.

public class Solution {
    public static int findEvenIndex(int[] arr) {
        long total = 0;
        for (int v : arr) {
            total += v;
        }
        long left = 0;
        for (int i = 0; i < arr.length; i++) {
            long right = total - left - arr[i];
            if (left == right) {
                return i;
            }
            left += arr[i];
        }
        return -1;
    }

    public static void main(String[] args) {
        int[] arr = {1, 2, 3, 4, 3, 2, 1};
        System.out.println(findEvenIndex(arr));
    }
}