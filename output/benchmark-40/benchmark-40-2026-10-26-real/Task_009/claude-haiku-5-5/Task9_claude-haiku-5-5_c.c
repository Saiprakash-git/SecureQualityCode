/* Determine whether the sum of integers is odd or even. */
#include <stdio.h>

const char *odd_or_even(const int *arr, size_t n) {
    long long sum = 0;
    if (arr == NULL || n == 0) {
        return "even";
    }
    for (size_t i = 0; i < n; i++) {
        sum += arr[i];
    }
    return (sum % 2 == 0) ? "even" : "odd";
}

int main(void) {
    int a1[] = {0};
    int a2[] = {0, 1, 4};
    int a3[] = {0, -1, -5};
    printf("%s\n", odd_or_even(a1, sizeof(a1) / sizeof(a1[0])));
    printf("%s\n", odd_or_even(a2, sizeof(a2) / sizeof(a2[0])));
    printf("%s\n", odd_or_even(a3, sizeof(a3) / sizeof(a3[0])));
    return 0;
}