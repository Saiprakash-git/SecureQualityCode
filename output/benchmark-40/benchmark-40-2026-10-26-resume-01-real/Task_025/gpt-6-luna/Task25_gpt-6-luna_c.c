#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char **number(size_t n, const char *const lines[])
{
    char **result = calloc(n ? n : 1, sizeof(*result));
    if (result == NULL)
        return NULL;

    for (size_t i = 0; i < n; ++i) {
        const char *line = lines[i] ? lines[i] : "";
        int prefix_length = snprintf(NULL, 0, "%zu: ", i + 1);
        if (prefix_length < 0) {
            for (size_t j = 0; j < i; ++j)
                free(result[j]);
            free(result);
            return NULL;
        }

        size_t line_length = strlen(line);
        result[i] = malloc((size_t)prefix_length + line_length + 1);
        if (result[i] == NULL) {
            for (size_t j = 0; j < i; ++j)
                free(result[j]);
            free(result);
            return NULL;
        }

        snprintf(result[i], (size_t)prefix_length + 1, "%zu: ", i + 1);
        memcpy(result[i] + prefix_length, line, line_length + 1);
    }

    return result;
}