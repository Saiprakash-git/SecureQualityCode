#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char **number_lines(const char *const lines[], size_t count) {
    char **result = malloc((count ? count : 1) * sizeof *result);
    if (!result) return NULL;

    for (size_t i = 0; i < count; i++) {
        int len = snprintf(NULL, 0, "%zu: %s", i + 1, lines[i]);
        if (len < 0) {
            for (size_t j = 0; j < i; j++) free(result[j]);
            free(result);
            return NULL;
        }
        result[i] = malloc((size_t)len + 1);
        if (!result[i]) {
            for (size_t j = 0; j < i; j++) free(result[j]);
            free(result);
            return NULL;
        }
        snprintf(result[i], (size_t)len + 1, "%zu: %s", i + 1, lines[i]);
    }
    return result;
}

void free_numbered_lines(char **lines, size_t count) {
    if (!lines) return;
    for (size_t i = 0; i < count; i++) free(lines[i]);
    free(lines);
}

int main(void) {
    const char *input[] = {"a", "b", "c"};
    size_t count = sizeof input / sizeof input[0];

    char **output = number_lines(input, count);
    if (!output && count > 0) {
        fprintf(stderr, "allocation failed\n");
        return 1;
    }

    for (size_t i = 0; i < count; i++) {
        printf("%s\n", output[i]);
    }

    free_numbered_lines(output, count);
    return 0;
}