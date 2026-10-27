public class ASum {
    public static long findNb(long m) {
        if (m <= 0) {
            return -1;
        }

        long s = isqrt(m);
        if (s * s != m) {
            return -1;
        }

        // We need n * (n + 1) / 2 = s => n * (n + 1) = 2 * s
        // Using quadratic formula: n = (-1 + sqrt(1 + 8 * s)) / 2
        // Since 8 * s can overflow long if s > Long.MAX_VALUE / 8,
        // let's binary search for n where n * (n + 1) == 2 * s.
        long target = 2 * s;
        long low = 1;
        long high = 2000000000L; // Since n*(n+1) ~ target, n <= 2 * 10^9

        while (low <= high) {
            long mid = low + (high - low) / 2;
            // Check mid * (mid + 1) vs target safely
            // mid + 1 cannot overflow since mid <= 2*10^9
            if (mid > target / (mid + 1)) {
                high = mid - 1;
            } else {
                long prod = mid * (mid + 1);
                if (prod == target) {
                    return mid;
                }
                low = mid + 1;
            }
        }

        return -1;
    }

    private static long isqrt(long n) {
        if (n < 0) {
            return -1;
        }
        if (n == 0) {
            return 0;
        }
        long x0 = (long) Math.sqrt(n);
        while (x0 > 0 && x0 > n / x0) {
            x0--;
        }
        while ((x0 + 1) <= n / (x0 + 1)) {
            x0++;
        }
        return x0;
    }
}