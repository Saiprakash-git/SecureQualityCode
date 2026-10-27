#include <stddef.h>

char *odd_or_even(const int *array, size_t length)
{
    int odd = 0;

    for (size_t i = 0; i < length; ++i) {
        odd ^= (array[i] % 2 != 0);
    }

    return odd ? "odd" : "even";
}