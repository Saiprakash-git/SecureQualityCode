#include <stdio.h>
#include <limits.h>

int maxSubArraySum(int arr[], int size, int *start, int *end) {
    int max_so_far = INT_MIN;
    int max_ending_here = 0;
    int s = 0;

    for (int i = 0; i < size; i++) {
        max_ending_here += arr[i];

        if (max_so_far < max_ending_here) {
            max_so_far = max_ending_here;
            *start = s;
            *end = i;
        }

        if (max_ending_here < 0) {
            max_ending_here = 0;
            s = i + 1;
        }
    }

    return max_so_far;
}

int main() {
    int n;
    scanf("%d", &n);

    int arr[n];
    for (int i = 0; i < n; i++) {
        scanf("%d", &arr[i]);
    }

    int start, end;
    int max_sum = maxSubArraySum(arr, n, &start, &end);

    printf("%d\n", max_sum);

    return 0;
}