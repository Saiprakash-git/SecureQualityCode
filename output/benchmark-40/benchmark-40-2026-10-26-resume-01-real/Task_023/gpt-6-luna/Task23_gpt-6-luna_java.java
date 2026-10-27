public class Kata {
    public static long findNb(long m) {
        if (m <= 0) {
            return -1;
        }

        long sum = 0;
        for (long n = 1; ; n++) {
            long cube = n * n * n;
            if (cube > m - sum) {
                return -1;
            }
            sum += cube;
            if (sum == m) {
                return n;
            }
        }
    }
}