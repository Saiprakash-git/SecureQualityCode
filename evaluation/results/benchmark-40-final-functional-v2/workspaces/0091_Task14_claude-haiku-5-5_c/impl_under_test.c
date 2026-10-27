#include <stdio.h>

int find_equilibrium(const int *arr, size_t n) {
    long long total = 0;
    for (size_t i = 0; i < n; i++) {
        total += arr[i];
    }
    long long left = 0;
    for (size_t i = 0; i < n; i++) {
        long long right = total - left - arr[i];
        if (left == right) {
            return (int)i;
        }
        left += arr[i];
    }
    return -1;
}

int main(void) {
    size_t n;
    if (scanf("%zu", &n) != 1) {
        return 1;
    }
    int arr[1000];
    if (n > 1000) {
        return 1;
    }
    for (size_t i = 0; i < n; i++) {
        if (scanf("%d", &arr[i]) != 1) {
            return 1;
        }
    }
    printf("%d\n", find_equilibrium(arr, n));
    return 0;
}