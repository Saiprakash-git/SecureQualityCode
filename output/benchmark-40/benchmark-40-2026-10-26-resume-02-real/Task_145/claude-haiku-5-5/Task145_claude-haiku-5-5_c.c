#include <stdio.h>
#include <stdlib.h>

typedef struct {
    long long maxSum;
    int start;
    int end;
} MaxSubarrayResult;

MaxSubarrayResult maxSubarray(const int *arr, int n) {
    MaxSubarrayResult res;
    res.maxSum = 0;
    res.start = 0;
    res.end = -1;

    if (arr == NULL || n <= 0) {
        return res;
    }

    long long currentSum = arr[0];
    long long bestSum = arr[0];
    int currentStart = 0;
    int bestStart = 0;
    int bestEnd = 0;

    for (int i = 1; i < n; i++) {
        if (currentSum < 0) {
            currentSum = arr[i];
            currentStart = i;
        } else {
            currentSum += arr[i];
        }

        if (currentSum > bestSum) {
            bestSum = currentSum;
            bestStart = currentStart;
            bestEnd = i;
        }
    }

    res.maxSum = bestSum;
    res.start = bestStart;
    res.end = bestEnd;
    return res;
}

int main(void) {
    int n;

    if (scanf("%d", &n) != 1 || n <= 0) {
        printf("No elements provided.\n");
        return 0;
    }

    int *arr = (int *)malloc(sizeof(int) * n);
    if (arr == NULL) {
        printf("Memory allocation failed.\n");
        return 1;
    }

    for (int i = 0; i < n; i++) {
        if (scanf("%d", &arr[i]) != 1) {
            printf("Invalid input.\n");
            free(arr);
            return 1;
        }
    }

    MaxSubarrayResult r = maxSubarray(arr, n);

    printf("Maximum subarray sum: %lld\n", r.maxSum);
    printf("Subarray indices: %d to %d\n", r.start, r.end);
    printf("Subarray elements:");
    for (int i = r.start; i <= r.end; i++) {
        printf(" %d", arr[i]);
    }
    printf("\n");

    free(arr);
    return 0;
}