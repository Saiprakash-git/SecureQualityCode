#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_BUF_SIZE 1024

static char *duplicate_string(const char *src, size_t len)
{
    char *dst = malloc(len + 1);
    if (dst == NULL) {
        return NULL;
    }
    memcpy(dst, src, len);
    dst[len] = '\0';
    return dst;
}

static char *read_line(FILE *fp)
{
    char buffer[LINE_BUF_SIZE];

    if (fgets(buffer, sizeof(buffer), fp) == NULL) {
        return NULL;
    }

    size_t len = strcspn(buffer, "\r\n");
    return duplicate_string(buffer, len);
}

static char *concatenate_strings(char **strings, size_t count)
{
    size_t total = 0;

    for (size_t i = 0; i < count; i++) {
        total += strlen(strings[i]);
    }

    char *result = malloc(total + 1);
    if (result == NULL) {
        return NULL;
    }

    char *pos = result;
    for (size_t i = 0; i < count; i++) {
        size_t len = strlen(strings[i]);
        memcpy(pos, strings[i], len);
        pos += len;
    }
    *pos = '\0';

    return result;
}

static size_t read_count(FILE *in, FILE *out)
{
    char buffer[LINE_BUF_SIZE];

    fprintf(out, "How many strings do you want to concatenate? ");
    fflush(out);

    if (fgets(buffer, sizeof(buffer), in) == NULL) {
        return 0;
    }

    char *end = NULL;
    unsigned long value = strtoul(buffer, &end, 10);
    if (end == buffer || value == 0) {
        return 0;
    }

    return (size_t)value;
}

int main(void)
{
    size_t count = read_count(stdin, stdout);
    if (count == 0) {
        fprintf(stderr, "Error: invalid count.\n");
        return EXIT_FAILURE;
    }

    char **strings = calloc(count, sizeof(char *));
    if (strings == NULL) {
        fprintf(stderr, "Error: memory allocation failed.\n");
        return EXIT_FAILURE;
    }

    for (size_t i = 0; i < count; i++) {
        printf("Enter string %zu: ", i + 1);
        fflush(stdout);

        strings[i] = read_line(stdin);
        if (strings[i] == NULL) {
            fprintf(stderr, "Error: failed to read input.\n");
            for (size_t j = 0; j < i; j++) {
                free(strings[j]);
            }
            free(strings);
            return EXIT_FAILURE;
        }
    }

    char *result = concatenate_strings(strings, count);
    if (result == NULL) {
        fprintf(stderr, "Error: memory allocation failed.\n");
        for (size_t i = 0; i < count; i++) {
            free(strings[i]);
        }
        free(strings);
        return EXIT_FAILURE;
    }

    printf("Concatenated result: %s\n", result);

    free(result);
    for (size_t i = 0; i < count; i++) {
        free(strings[i]);
    }
    free(strings);

    return EXIT_SUCCESS;
}