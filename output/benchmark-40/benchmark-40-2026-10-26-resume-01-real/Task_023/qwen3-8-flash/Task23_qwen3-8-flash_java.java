public class Solution {
    public static long findNb(long m) {
        if (m < 0) {
            return -1;
        }

        long sqrtM = integerSqrt(m);
        if (sqrtM * sqrtM != m) {
            return -1;
        }

        long triangular = sqrtM;
        long discriminant = 8L * triangular + 1L;
        long sqrtD = integerSqrt(discriminant);

        if (sqrtD * sqrtD != discriminant) {
            return -1;
        }

        if ((sqrtD - 1L) % 2L != 0L) {
            return -1;
        }

        long n = (sqrtD - 1L) / 2L;

        if (n * (n + 1L) / 2L == triangular) {
            return n;
        }

        return -1;
    }

    private static long integerSqrt(long x) {
        long low = 0L;
        long high = 3037000499L;
        long answer = 0L;

        while (low <= high) {
            long mid = low + (high - low) / 2L;
            long square = mid * mid;

            if (square == x) {
                return mid;
            } else if (square < x) {
                answer = mid;
                low = mid + 1L;
            } else {
                high = mid - 1L;
            }
        }

        return answer;
    }
}