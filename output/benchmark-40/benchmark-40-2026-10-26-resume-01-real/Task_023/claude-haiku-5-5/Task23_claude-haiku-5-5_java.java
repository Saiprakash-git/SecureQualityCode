public class Solution {
    public static long findNb(long m) {
        long sum = 0;
        long n = 0;
        while (sum < m) {
            n++;
            sum += n * n * n;
        }
        return sum == m ? n : -1;
    }

    public static void main(String[] args) {
        System.out.println(findNb(1071225));
        System.out.println(findNb(91716553919377L));
    }
}