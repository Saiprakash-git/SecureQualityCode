#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void free_strings(char **strings, size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        free(strings[i]);
    }
    free(strings);
}

static char *concatenate_strings(const char *const *strings, size_t count)
{
    size_t total_length = 0;

    for (size_t i = 0; i < count; ++i) {
        if (strings[i] != NULL) {
            total_length += strlen(strings[i]);
        }
    }

    char *result = malloc(total_length + 1);
    if (result == NULL) {
        return NULL;
    }

    result[0] = '\0';
    char *write_position = result;

    for (size_t i = 0; i < count; ++i) {
        if (strings[i] != NULL) {
            size_t length = strlen(strings[i]);
            memcpy(write_position, strings[i], length);
            write_position += length;
        }
    }

    *write_position = '\0';
    return result;
}

int main(void)
{
    char buffer[4096];
    char **strings = NULL;
    size_t count = 0;
    size_t capacity = 0;

    while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        size_t length = strlen(buffer);

        if (length > 0 && buffer[length - 1] == '\n') {
            buffer[--length] = '\0';

            if (length > 0 && buffer[length - 1] == '\r') {
                buffer[--length] = '\0';
            }
        }

        if (count == capacity) {
            size_t new_capacity = (capacity == 0) ? 8 : capacity * 2;
            char **tmp = realloc(strings, new_capacity * sizeof(*tmp));

            if (tmp == NULL) {
                free_strings(strings, count);
                return 1;
            }

            strings = tmp;
            capacity = new_capacity;
        }

        char *copy = malloc(length + 1);
        if (copy == NULL) {
            free_strings(strings, count);
            return 1;
        }

        memcpy(copy, buffer, length + 1);
        strings[count++] = copy;
    }

    char *result = concatenate_strings(strings, count);

    if (result == NULL) {
        free_strings(strings, count);
        return 1;
    }

    printf("%s", result);

    if (count > 0) {
        putchar('\n');
    }

    free(result);
    free_strings(strings, count);

    return 0;
}