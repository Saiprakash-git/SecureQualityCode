#include <cmath>

long long integer_sqrt(long long x) {
    if (x < 0) return -1;
    if (x == 0) return 0;
    long long left = 1, right = 3037000499LL; // sqrt(LLONG_MAX)
    if (right > x) right = x;
    long long ans = 0;
    while (left <= right) {
        long long mid = left + (right - left) / 2;
        if (mid <= x / mid) {
            ans = mid;
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    return ans;
}

long long findNb(long long m) {
    long long s = integer_sqrt(m);
    if (s * s != m) {
        return -1;
    }
    // Now n*(n+1) = 2*s
    // n^2 + n - 2*s = 0
    // Discriminant = 1 + 8*s
    // To avoid overflow in 8*s: s <= 2*10^9, so 8*s <= 1.6*10^10, fits comfortably in long long.
    long long disc = 1 + 8 * s;
    long long d = integer_sqrt(disc);
    if (d * d != disc) {
        return -1;
    }
    if ((d - 1) % 2 != 0) {
        return -1;
    }
    long long n = (d - 1) / 2;
    if (n > 0) {
        return n;
    }
    return -1;
}