#include <stdio.h>

void sort_array(int arr[], int size) {
    for (int i = 1; i < size; i++) {
        int key = arr[i];
        int j = i - 1;
        while (j >= 0 && arr[j] > key) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = key;
    }
}

int main() {
    int arr[1000];
    int size = 0;
    int num;

    while (scanf("%d", &num) == 1) {
        if (size < 1000) {
            arr[size++] = num;
        }
    }

    sort_array(arr, size);

    for (int i = 0; i < size; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");

    return 0;
}