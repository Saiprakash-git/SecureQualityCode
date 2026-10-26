#include <stddef.h>

int binary_array_to_number(const unsigned char *arr, size_t len)
{
    int value = 0;

    for (size_t i = 0; i < len; ++i) {
        value = value * 2 + arr[i];
    }

    return value;
}