long long findNb(long long m) {
    if (m <= 0) {
        return -1;
    }

    long long low = 1, high = 2000000;
    while (low <= high) {
        long long mid = low + (high - low) / 2;
        unsigned long long sum = (unsigned long long)mid * (mid + 1) / 2;
        unsigned long long sum_sq = sum * sum;

        if (sum_sq == (unsigned long long)m) {
            return mid;
        } else if (sum_sq < (unsigned long long)m) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }

    return -1;
}