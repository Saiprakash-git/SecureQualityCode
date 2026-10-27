long long isqrt(long long x) {
    if (x < 0) return -1;
    if (x == 0) return 0;
    long long lo = 1, hi = x, ans = 0;
    while (lo <= hi) {
        long long mid = lo + (hi - lo) / 2;
        if (mid <= x / mid) {
            ans = mid;
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }
    return ans;
}

long long findNb(long long m) {
    if (m < 1) return -1;
    long long s = isqrt(m);
    if (s * s != m) return -1;
    long long disc = 1 + 8 * s;
    long long t = isqrt(disc);
    if (t * t != disc) return -1;
    if ((t - 1) % 2 != 0) return -1;
    return (t - 1) / 2;
}