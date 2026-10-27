#include <limits.h>

long long find_nb(long long m)
{
    if (m <= 0)
        return -1;

    long long sum = 0;

    for (long long n = 1; ; ++n) {
        long long remaining = m - sum;

        if (n > remaining / n / n)
            return -1;

        long long cube = n * n * n;
        sum += cube;

        if (sum == m)
            return n;
    }
}

long long findNb(long long m)
{
    return find_nb(m);
}