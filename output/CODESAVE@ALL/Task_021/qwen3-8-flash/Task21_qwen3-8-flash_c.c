#include <stdlib.h>

int *remove_smallest(int *array, int size, int *new_size)
{
    int out_size;
    int *out;
    int min_idx;
    int i;
    int j;

    out_size = (size > 0) ? size - 1 : 0;

    if (new_size != NULL) {
        *new_size = out_size;
    }

    out = (int *)malloc((out_size > 0 ? (size_t)out_size : 1) * sizeof(int));
    if (out == NULL) {
        return NULL;
    }

    if (size <= 0 || array == NULL) {
        return out;
    }

    min_idx = 0;
    for (i = 1; i < size; ++i) {
        if (array[i] < array[min_idx]) {
            min_idx = i;
        }
    }

    j = 0;
    for (i = 0; i < size; ++i) {
        if (i != min_idx) {
            out[j] = array[i];
            ++j;
        }
    }

    return out;
}