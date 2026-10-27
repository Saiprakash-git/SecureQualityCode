#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>

#define BUFFER_SIZE 64
#define MAX_ELEMENTS 100
#define MIN_ELEMENTS 1

int read_input_line(char *buffer, size_t size, FILE *stream) {
    if (buffer == NULL || size == 0 || stream == NULL) {
        return 0;
    }

    if (fgets(buffer, (int)size, stream) == NULL) {
        return 0;
    }

    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    } else {
        int ch;
        while ((ch = fgetc(stream)) != '\n' && ch != EOF);
        if (len == size - 1) {
            return 0;
        }
    }

    return 1;
}

int parse_integer(const char *str, long min_val, long max_val, long *out_val) {
    if (str == NULL || out_val == NULL) {
        return 0;
    }

    while (isspace((unsigned char)*str)) {
        str++;
    }

    if (*str == '\0') {
        return 0;
    }

    char *endptr = NULL;
    errno = 0;
    long val = strtol(str, &endptr, 10);

    if (errno == ERANGE || errno != 0) {
        return 0;
    }

    if (endptr == str) {
        return 0;
    }

    while (isspace((unsigned char)*endptr)) {
        endptr++;
    }

    if (*endptr != '\0') {
        return 0;
    }

    if (val < min_val || val > max_val) {
        return 0;
    }

    *out_val = val;
    return 1;
}

int safe_add(long a, long b, long *result) {
    if (result == NULL) {
        return 0;
    }

    if ((b > 0 && a > LONG_MAX - b) || (b < 0 && a < LONG_MIN - b)) {
        return 0;
    }

    *result = a + b;
    return 1;
}

int calculate_sum(const long *array, size_t count, long *out_sum) {
    if (array == NULL || out_sum == NULL || count == 0) {
        return 0;
    }

    long total = 0;
    for (size_t i = 0; i < count; i++) {
        if (!safe_add(total, array[i], &total)) {
            return 0;
        }
    }

    *out_sum = total;
    return 1;
}

int find_min_max(const long *array, size_t count, long *out_min, long *out_max) {
    if (array == NULL || out_min == NULL || out_max == NULL || count == 0) {
        return 0;
    }

    long min = array[0];
    long max = array[0];

    for (size_t i = 1; i < count; i++) {
        if (array[i] < min) {
            min = array[i];
        }
        if (array[i] > max) {
            max = array[i];
        }
    }

    *out_min = min;
    *out_max = max;
    return 1;
}

int compare_longs(const void *a, const void *b) {
    long arg1 = *(const long *)a;
    long arg2 = *(const long *)b;

    if (arg1 < arg2) return -1;
    if (arg1 > arg2) return 1;
    return 0;
}

int sort_array(long *array, size_t count) {
    if (array == NULL || count == 0) {
        return 0;
    }

    qsort(array, count, sizeof(long), compare_longs);
    return 1;
}

int main(void) {
    char input_buffer[BUFFER_SIZE];
    long count = 0;

    printf("Enter number of elements (%d to %d): ", MIN_ELEMENTS, MAX_ELEMENTS);
    if (!read_input_line(input_buffer, sizeof(input_buffer), stdin)) {
        fprintf(stderr, "Error: Failed to read element count.\n");
        return EXIT_FAILURE;
    }

    if (!parse_integer(input_buffer, MIN_ELEMENTS, MAX_ELEMENTS, &count)) {
        fprintf(stderr, "Error: Invalid count. Must be an integer between %d and %d.\n", MIN_ELEMENTS, MAX_ELEMENTS);
        return EXIT_FAILURE;
    }

    long numbers[MAX_ELEMENTS];

    for (long i = 0; i < count; i++) {
        printf("Enter element %ld (%ld to %ld): ", i + 1, LONG_MIN, LONG_MAX);
        if (!read_input_line(input_buffer, sizeof(input_buffer), stdin)) {
            fprintf(stderr, "Error: Failed to read input.\n");
            return EXIT_FAILURE;
        }

        long val = 0;
        if (!parse_integer(input_buffer, LONG_MIN, LONG_MAX, &val)) {
            fprintf(stderr, "Error: Invalid number entered.\n");
            return EXIT_FAILURE;
        }

        numbers[i] = val;
    }

    long sum = 0;
    int sum_ok = calculate_sum(numbers, (size_t)count, &sum);

    long min_val = 0;
    long max_val = 0;
    if (!find_min_max(numbers, (size_t)count, &min_val, &max_val)) {
        fprintf(stderr, "Error: Failed to determine min and max.\n");
        return EXIT_FAILURE;
    }

    if (!sort_array(numbers, (size_t)count)) {
        fprintf(stderr, "Error: Failed to sort elements.\n");
        return EXIT_FAILURE;
    }

    printf("\n--- Results ---\n");
    if (sum_ok) {
        printf("Sum: %ld\n", sum);
        printf("Average: %.2f\n", (double)sum / (double)count);
    } else {
        printf("Sum: Overflow occurred during calculation\n");
        printf("Average: N/A\n");
    }

    printf("Minimum: %ld\n", min_val);
    printf("Maximum: %ld\n", max_val);

    printf("Sorted elements: ");
    for (long i = 0; i < count; i++) {
        printf("%ld%s", numbers[i], (i == count - 1) ? "\n" : ", ");
    }

    return EXIT_SUCCESS;
}