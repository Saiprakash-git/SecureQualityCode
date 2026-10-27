#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

static int append_bytes(char **buffer, size_t *capacity, size_t *length,
                        const char *data, size_t data_length)
{
    if (data_length > SIZE_MAX - *length - 1)
        return 0;

    size_t needed = *length + data_length + 1;
    if (needed > *capacity) {
        size_t new_capacity = *capacity ? *capacity : 16;
        while (new_capacity < needed) {
            if (new_capacity > SIZE_MAX / 2) {
                new_capacity = needed;
                break;
            }
            new_capacity *= 2;
        }

        char *new_buffer = realloc(*buffer, new_capacity);
        if (!new_buffer)
            return 0;

        *buffer = new_buffer;
        *capacity = new_capacity;
    }

    for (size_t i = 0; i < data_length; ++i)
        (*buffer)[*length + i] = data[i];

    *length += data_length;
    (*buffer)[*length] = '\0';
    return 1;
}

int main(void)
{
    char *result = NULL;
    char *line = NULL;
    size_t result_capacity = 0, result_length = 0;
    size_t line_capacity = 0, line_length = 0;
    int ch;

    while ((ch = getchar()) != EOF) {
        if (ch == '\n') {
            if (line_length > 0 && line[line_length - 1] == '\r')
                --line_length;

            if (!append_bytes(&result, &result_capacity, &result_length,
                              line, line_length))
                goto failure;

            line_length = 0;
            if (line)
                line[0] = '\0';
        } else {
            char character = (char)ch;
            if (!append_bytes(&line, &line_capacity, &line_length,
                              &character, 1))
                goto failure;
        }
    }

    if (line_length > 0 &&
        !append_bytes(&result, &result_capacity, &result_length,
                      line, line_length))
        goto failure;

    if (result)
        fputs(result, stdout);
    putchar('\n');

    free(line);
    free(result);
    return 0;

failure:
    free(line);
    free(result);
    return 1;
}