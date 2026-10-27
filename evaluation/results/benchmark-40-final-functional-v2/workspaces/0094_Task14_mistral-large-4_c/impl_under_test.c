#include <stdio.h>

int find_even_index(int arr[], int length) {
    int total_sum = 0;
    int left_sum = 0;
    int i;

    for (i = 0; i < length; i++) {
        total_sum += arr[i];
    }

    for (i = 0; i < length; i++) {
        int right_sum = total_sum - left_sum - arr[i];
        if (left_sum == right_sum) {
            return i;
        }
        left_sum += arr[i];
    }

    return -1;
}

int main() {
    int test1[] = {1, 2, 3, 4, 3, 2, 1};
    int test2[] = {1, 100, 50, -51, 1, 1};
    int test3[] = {20, 10, -80, 10, 10, 15, 35};

    printf("%d\n", find_even_index(test1, 7));
    printf("%d\n", find_even_index(test2, 6));
    printf("%d\n", find_even_index(test3, 7));

    return 0;
}