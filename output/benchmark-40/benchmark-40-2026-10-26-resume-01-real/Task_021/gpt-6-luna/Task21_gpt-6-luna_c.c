#include <stddef.h>
#include <stdlib.h>

int *remove_smallest(const int *numbers, size_t numbers_size, size_t *result_size)
{
    if (result_size == NULL)
        return NULL;

    *result_size = 0;

    if (numbers_size == 0 || numbers == NULL)
        return NULL;

    size_t smallest_index = 0;
    for (size_t i = 1; i < numbers_size; ++i) {
        if (numbers[i] < numbers[smallest_index])
            smallest_index = i;
    }

    *result_size = numbers_size - 1;
    if (numbers_size == 1)
        return NULL;

    int *result = malloc((numbers_size - 1) * sizeof(*result));
    if (result == NULL) {
        *result_size = 0;
        return NULL;
    }

    size_t output_index = 0;
    for (size_t i = 0; i < numbers_size; ++i) {
        if (i != smallest_index)
            result[output_index++] = numbers[i];
    }

    return result;
}