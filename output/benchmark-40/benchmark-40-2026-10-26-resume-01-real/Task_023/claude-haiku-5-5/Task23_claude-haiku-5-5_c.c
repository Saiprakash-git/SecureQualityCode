#include <stdio.h>

long long findNb(long long m) {
    long long n = 0;
    long long sum = 0;
    while (sum < m) {
        n++;
        sum += n * n * n;
    }
    if (sum == m) {
        return n;
    }
    return -1;
}

int main(void) {
    printf("%lld\n", findNb(1071225));
    printf("%lld\n", findNb(91716553919377LL));
    return 0;
}