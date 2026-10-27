#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char** number_lines(char** lines, size_t count) {
    if (count == 0 || lines == NULL) {
        return NULL;
    }

    char** result = malloc(count * sizeof(char*));
    if (!result) {
        return NULL;
    }

    for (size_t i = 0; i < count; i++) {
        const char* line = lines[i] ? lines[i] : "";

        int needed = snprintf(NULL, 0, "%zu: %s", i + 1, line);
        if (needed < 0) {
            for (size_t j = 0; j < i; j++) {
                free(result[j]);
            }
            free(result);
            return NULL;
        }

        result[i] = malloc((size_t)needed + 1);
        if (!result[i]) {
            for (size_t j = 0; j < i; j++) {
                free(result[j]);
            }
            free(result);
            return NULL;
        }

        snprintf(result[i], (size_t)needed + 1, "%zu: %s", i + 1, line);
    }

    return result;
}

void free_numbered_lines(char** lines, size_t count) {
    if (!lines) {
        return;
    }
    for (size_t i = 0; i < count; i++) {
        free(lines[i]);
    }
    free(lines);
}

int main(void) {
    char* input[] = {"a", "b", "c"};
    size_t count = 3;

    char** output = number_lines(input, count);
    if (output) {
        for (size_t i = 0; i < count; i++) {
            printf("%s\n", output[i]);
        }
        free_numbered_lines(output, count);
    }

    char** empty = number_lines(NULL, 0);
    printf("Empty result is %s\n", empty ? "not NULL" : "NULL");

    return 0;
}