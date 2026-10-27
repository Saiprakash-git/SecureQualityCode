#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

typedef struct {
    long long sum;
    size_t start;
    size_t end;
    int valid;
} SubarrayResult;

static SubarrayResult max_subarray(const int *values, size_t count)
{
    SubarrayResult result = {0, 0, 0, 0};

    if (values == NULL || count == 0) {
        return result;
    }

    long long current_sum = values[0];
    result.sum = current_sum;
    result.start = 0;
    result.end = 0;
    result.valid = 1;

    size_t current_start = 0;

    for (size_t i = 1; i < count; ++i) {
        if (current_sum < 0) {
            current_sum = values[i];
            current_start = i;
        } else {
            current_sum += values[i];
        }

        if (current_sum > result.sum) {
            result.sum = current_sum;
            result.start = current_start;
            result.end = i;
        }
    }

    return result;
}

int main(void)
{
    size_t count;

    if (scanf("%zu", &count) != 1) {
        return 1;
    }

    if (count == 0) {
        printf("No subarray\n");
        return 0;
    }

    int *values = malloc(count * sizeof(*values));
    if (values == NULL) {
        return 1;
    }

    for (size_t i = 0; i < count; ++i) {
        if (scanf("%d", &values[i]) != 1) {
            free(values);
            return 1;
        }
    }

    SubarrayResult result = max_subarray(values, count);

    printf("Maximum sum: %lld\nSubarray:", result.sum);
    for (size_t i = result.start; i <= result.end; ++i) {
        printf(" %d", values[i]);
    }
    putchar('\n');

    free(values);
    return 0;
}