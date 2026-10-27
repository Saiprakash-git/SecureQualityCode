#include <stddef.h>

int find_even_index(const int *values, size_t length)
{
    long long total = 0;
    long long left = 0;

    for (size_t i = 0; i < length; ++i)
        total += values[i];

    for (size_t i = 0; i < length; ++i) {
        if (left == total - left - values[i])
            return (int)i;
        left += values[i];
    }

    return -1;
}