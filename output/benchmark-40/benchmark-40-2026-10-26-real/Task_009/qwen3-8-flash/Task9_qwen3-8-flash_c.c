#include <stddef.h>

char *odd_or_even(int *arr, int n) {
    int parity = 0;

    if (arr == NULL || n <= 0) {
        return "even";
    }

    for (int i = 0; i < n; ++i) {
        if (arr[i] % 2 != 0) {
            parity ^= 1;
        }
    }

    return parity ? "odd" : "even";
}