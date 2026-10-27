#include <stdio.h>
#include <stdlib.h>

size_t remove_smallest(const int *arr, size_t n, int *out) {
    if (n == 0) {
        return 0;
    }

    size_t min_idx = 0;
    for (size_t i = 1; i < n; i++) {
        if (arr[i] < arr[min_idx]) {
            min_idx = i;
        }
    }

    size_t j = 0;
    for (size_t i = 0; i < n; i++) {
        if (i != min_idx) {
            out[j++] = arr[i];
        }
    }
    return j;
}

static void print_array(const int *arr, size_t n) {
    printf("[");
    for (size_t i = 0; i < n; i++) {
        printf("%d", arr[i]);
        if (i + 1 < n) {
            printf(",");
        }
    }
    printf("]\n");
}

int main(void) {
    int t1[] = {1, 2, 3, 4, 5};
    int t2[] = {5, 3, 2, 1, 4};
    int t3[] = {2, 2, 1, 2, 1};

    int *tests[] = {t1, t2, t3};
    size_t sizes[] = {5, 5, 5};

    for (size_t k = 0; k < 3; k++) {
        int *out = malloc(sizes[k] * sizeof(int));
        if (out == NULL) {
            return 1;
        }
        size_t len = remove_smallest(tests[k], sizes[k], out);
        print_array(out, len);
        free(out);
    }

    return 0;
}