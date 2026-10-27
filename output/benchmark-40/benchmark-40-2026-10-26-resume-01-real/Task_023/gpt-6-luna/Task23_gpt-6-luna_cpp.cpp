long long find_nb(long long m) {
    if (m < 0) return -1;

    __int128 total = 0;
    for (long long n = 1; total < m; ++n) {
        total += static_cast<__int128>(n) * n * n;
        if (total == m) return n;
    }

    return -1;
}