#include <stddef.h>

const char *odd_or_even(const int *arr, size_t n) {
    int odd_count = 0;
    for (size_t i = 0; i < n; i++) {
        if (arr[i] % 2 != 0) {
            odd_count++;
        }
    }
    return (odd_count % 2 != 0) ? "odd" : "even";
}