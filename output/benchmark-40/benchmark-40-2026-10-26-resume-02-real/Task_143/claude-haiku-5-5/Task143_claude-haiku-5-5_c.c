#include <stdio.h>
#include <stdlib.h>

void sort_ascending(int *arr, size_t n) {
    for (size_t i = 1; i < n; i++) {
        int key = arr[i];
        size_t j = i;
        while (j > 0 && arr[j - 1] > key) {
            arr[j] = arr[j - 1];
            j--;
        }
        arr[j] = key;
    }
}

int main(void) {
    size_t n;
    if (scanf("%zu", &n) != 1) {
        return 1;
    }

    int *arr = malloc(n * sizeof(int));
    if (arr == NULL && n > 0) {
        return 1;
    }

    for (size_t i = 0; i < n; i++) {
        if (scanf("%d", &arr[i]) != 1) {
            free(arr);
            return 1;
        }
    }

    sort_ascending(arr, n);

    for (size_t i = 0; i < n; i++) {
        printf("%d", arr[i]);
        if (i + 1 < n) {
            printf(" ");
        }
    }
    printf("\n");

    free(arr);
    return 0;
}