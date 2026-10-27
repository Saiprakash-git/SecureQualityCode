#include <stddef.h>

const char* sum_parity(const int* arr, size_t len) {
    int is_odd = 0;
    size_t i;
    for (i = 0; i < len; i++) {
        is_odd ^= (arr[i] % 2 != 0);
    }
    return is_odd ? "odd" : "even";
}