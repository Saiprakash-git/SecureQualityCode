#include <stddef.h>
#include <stdio.h>

int find_even_index(const int *values, int length) {
    if (values == NULL || length <= 0) {
        return -1;
    }

    long long total_sum = 0;
    for (int i = 0; i < length; i++) {
        total_sum += values[i];
    }

    long long left_sum = 0;
    for (int i = 0; i < length; i++) {
        long long right_sum = total_sum - left_sum - values[i];
        if (left_sum == right_sum) {
            return i;
        }
        left_sum += values[i];
    }

    return -1;
}

int main(void) {
    int arr1[] = {1, 2, 3, 4, 3, 2, 1};
    printf("%d\n", find_even_index(arr1, sizeof(arr1) / sizeof(arr1[0])));

    int arr2[] = {1, 100, 50, -51, 1, 1};
    printf("%d\n", find_even_index(arr2, sizeof(arr2) / sizeof(arr2[0])));

    int arr3[] = {20, 10, -80, 10, 10, 15, 35};
    printf("%d\n", find_even_index(arr3, sizeof(arr3) / sizeof(arr3[0])));

    return 0;
}