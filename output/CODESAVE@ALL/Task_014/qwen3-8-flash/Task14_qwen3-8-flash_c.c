#include <stddef.h>

int find_even_index(int arr[], int size) {
    long long total = 0;
    long long left = 0;
    long long right = 0;
    int i;

    if (arr == NULL || size <= 0) {
        return -1;
    }

    for (i = 0; i < size; i++) {
        total += arr[i];
    }

    right = total;

    for (i = 0; i < size; i++) {
        right -= arr[i];

        if (left == right) {
            return i;
        }

        left += arr[i];
    }

    return -1;
}