#include <stdlib.h>
#include <stddef.h>

int* remove_smallest(const int* arr, size_t size, size_t* out_size) {
    if (size == 0) {
        *out_size = 0;
        return NULL;
    }

    size_t min_idx = 0;
    for (size_t i = 1; i < size; i++) {
        if (arr[i] < arr[min_idx]) {
            min_idx = i;
        }
    }

    size_t new_size = size - 1;
    int* result = (int*)malloc(new_size * sizeof(int));
    if (result == NULL) {
        *out_size = 0;
        return NULL;
    }

    for (size_t i = 0; i < min_idx; i++) {
        result[i] = arr[i];
    }

    for (size_t i = min_idx + 1; i < size; i++) {
        result[i - 1] = arr[i];
    }

    *out_size = new_size;
    return result;
}