#include <stdio.h>
#include <stdlib.h>

long long find_max_subarray(const int *arr, size_t n, size_t *out_start, size_t *out_end) {
    if (n == 0) {
        if (out_start) *out_start = 0;
        if (out_end) *out_end = 0;
        return 0;
    }

    long long max_so_far = arr[0];
    long long current_max = arr[0];
    size_t start = 0;
    size_t end = 0;
    size_t temp_start = 0;

    for (size_t i = 1; i < n; i++) {
        if ((long long)arr[i] > current_max + arr[i]) {
            current_max = arr[i];
            temp_start = i;
        } else {
            current_max += arr[i];
        }

        if (current_max > max_so_far) {
            max_so_far = current_max;
            start = temp_start;
            end = i;
        }
    }

    if (out_start) *out_start = start;
    if (out_end) *out_end = end;

    return max_so_far;
}

int main(void) {
    size_t n;
    if (scanf("%zu", &n) != 1 || n == 0) {
        return 0;
    }

    int *arr = (int *)malloc(n * sizeof(int));
    if (!arr) {
        return 1;
    }

    for (size_t i = 0; i < n; i++) {
        if (scanf("%d", &arr[i]) != 1) {
            free(arr);
            return 1;
        }
    }

    size_t start = 0, end = 0;
    long long max_sum = find_max_subarray(arr, n, &start, &end);

    printf("Maximum Contiguous Sum: %lld\n", max_sum);
    printf("Starting Index: %zu\n", start);
    printf("Ending Index: %zu\n", end);
    printf("Subarray: [");
    for (size_t i = start; i <= end; i++) {
        printf("%d%s", arr[i], (i == end) ? "" : ", ");
    }
    printf("]\n");

    free(arr);
    return 0;
}