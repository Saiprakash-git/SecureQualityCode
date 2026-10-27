#include <stdio.h>
#include <stdlib.h>

long long maxSubarraySum(const int arr[], int n, int *start, int *end)
{
    if (n <= 0 || arr == NULL || start == NULL || end == NULL) {
        if (start != NULL) *start = 0;
        if (end != NULL) *end = -1;
        return 0;
    }

    long long bestSum = arr[0];
    long long currentSum = arr[0];

    int bestStart = 0;
    int bestEnd = 0;
    int currentStart = 0;

    for (int i = 1; i < n; i++) {
        if (currentSum + arr[i] < arr[i]) {
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

    *start = bestStart;
    *end = bestEnd;

    return bestSum;
}

int main(void)
{
    int n;

    if (scanf("%d", &n) != 1 || n <= 0) {
        return 0;
    }

    int *arr = malloc((size_t)n * sizeof(int));
    if (arr == NULL) {
        return 1;
    }

    for (int i = 0; i < n; i++) {
        if (scanf("%d", &arr[i]) != 1) {
            arr[i] = 0;
        }
    }

    int start = 0;
    int end = 0;

    long long maxSum = maxSubarraySum(arr, n, &start, &end);

    printf("%lld\n", maxSum);

    for (int i = start; i <= end; i++) {
        printf("%d", arr[i]);
        if (i < end) {
            printf(" ");
        }
    }

    printf("\n");

    free(arr);

    return 0;
}