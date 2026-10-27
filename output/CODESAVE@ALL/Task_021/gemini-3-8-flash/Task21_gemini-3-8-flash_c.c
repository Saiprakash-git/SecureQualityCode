#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

int *remove_smallest(size_t length, const int *numbers) {
    if (length == 0 || numbers == NULL) {
        return NULL;
    }

    if (length == 1) {
        return (int *)malloc(0);
    }

    size_t min_idx = 0;
    for (size_t i = 1; i < length; i++) {
        if (numbers[i] < numbers[min_idx]) {
            min_idx = i;
        }
    }

    int *result = (int *)malloc((length - 1) * sizeof(int));
    if (result == NULL) {
        return NULL;
    }

    size_t dest_idx = 0;
    for (size_t i = 0; i < length; i++) {
        if (i != min_idx) {
            result[dest_idx++] = numbers[i];
        }
    }

    return result;
}

static void print_array(const int *arr, size_t length) {
    printf("[");
    for (size_t i = 0; i < length; i++) {
        printf("%d%s", arr[i], (i + 1 < length) ? ", " : "");
    }
    printf("]\n");
}

int main(void) {
    const int test1[] = {1, 2, 3, 4, 5};
    size_t len1 = sizeof(test1) / sizeof(test1[0]);
    int *res1 = remove_smallest(len1, test1);
    print_array(res1, len1 - 1);
    free(res1);

    const int test2[] = {5, 3, 2, 1, 4};
    size_t len2 = sizeof(test2) / sizeof(test2[0]);
    int *res2 = remove_smallest(len2, test2);
    print_array(res2, len2 - 1);
    free(res2);

    const int test3[] = {2, 2, 1, 2, 1};
    size_t len3 = sizeof(test3) / sizeof(test3[0]);
    int *res3 = remove_smallest(len3, test3);
    print_array(res3, len3 - 1);
    free(res3);

    return 0;
}