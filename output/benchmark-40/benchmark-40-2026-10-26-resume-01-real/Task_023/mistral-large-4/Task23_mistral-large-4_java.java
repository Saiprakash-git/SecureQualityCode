public class FindNb {
    public static long findNb(long m) {
        if (m < 1) {
            return -1;
        }

        long s = isqrt(m);
        if (s * s != m) {
            return -1;
        }

        long discriminant = 1 + 8 * s;
        long d = isqrt(discriminant);
        if (d * d != discriminant) {
            return -1;
        }

        if ((d - 1) % 2 != 0) {
            return -1;
        }

        long n = (d - 1) / 2;

        if (n * (n + 1) / 2 == s) {
            return n;
        }

        return -1;
    }

    private static long isqrt(long x) {
        if (x < 0) {
            return -1;
        }
        if (x < 2) {
            return x;
        }

        long left = 1;
        long right = Math.min(x, 1L << 32);

        while (left <= right) {
            long mid = left + (right - left) / 2;
            long sq = mid * mid;
            if (sq == x) {
                return mid;
            } else if (sq < x) {
                left = mid + 1;
            } else {
                right = mid - 1;
            }
        }
        return right;
    }
}