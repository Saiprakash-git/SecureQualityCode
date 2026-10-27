#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char **number_lines(const char *const *lines, size_t count)
{
    if (count == 0) {
        return NULL;
    }

    char **result = malloc(count * sizeof(char *));
    if (!result) {
        return NULL;
    }

    for (size_t i = 0; i < count; i++) {
        int len = snprintf(NULL, 0, "%zu: %s", i + 1, lines[i]);
        if (len < 0) {
            for (size_t j = 0; j < i; j++) {
                free(result[j]);
            }
            free(result);
            return NULL;
        }

        result[i] = malloc((size_t)len + 1);
        if (!result[i]) {
            for (size_t j = 0; j < i; j++) {
                free(result[j]);
            }
            free(result);
            return NULL;
        }

        snprintf(result[i], (size_t)len + 1, "%zu: %s", i + 1, lines[i]);
    }

    return result;
}

void free_numbered_lines(char **lines, size_t count)
{
    if (!lines) {
        return;
    }
    for (size_t i = 0; i < count; i++) {
        free(lines[i]);
    }
    free(lines);
}

int main(void)
{
    const char *lines[] = {"a", "b", "c"};
    size_t count = sizeof(lines) / sizeof(lines[0]);

    char **numbered = number_lines(lines, count);
    if (numbered) {
        for (size_t i = 0; i < count; i++) {
            printf("%s\n", numbered[i]);
        }
        free_numbered_lines(numbered, count);
    }

    return 0;
}