#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *concatenate_strings(char **strings, size_t count) {
    if (strings == NULL || count == 0) {
        char *empty = (char *)malloc(1);
        if (empty != NULL) {
            empty[0] = '\0';
        }
        return empty;
    }

    size_t total_length = 0;
    for (size_t i = 0; i < count; i++) {
        if (strings[i] != NULL) {
            total_length += strlen(strings[i]);
        }
    }

    char *result = (char *)malloc(total_length + 1);
    if (result == NULL) {
        return NULL;
    }

    result[0] = '\0';
    for (size_t i = 0; i < count; i++) {
        if (strings[i] != NULL) {
            strcat(result, strings[i]);
        }
    }

    return result;
}

char *read_line(FILE *stream) {
    size_t capacity = 128;
    size_t length = 0;
    char *buffer = (char *)malloc(capacity);
    if (buffer == NULL) {
        return NULL;
    }

    int ch;
    while ((ch = fgetc(stream)) != EOF && ch != '\n') {
        if (length + 1 >= capacity) {
            capacity *= 2;
            char *new_buffer = (char *)realloc(buffer, capacity);
            if (new_buffer == NULL) {
                free(buffer);
                return NULL;
            }
            buffer = new_buffer;
        }
        buffer[length++] = (char)ch;
    }

    if (length == 0 && ch == EOF) {
        free(buffer);
        return NULL;
    }

    buffer[length] = '\0';
    return buffer;
}

int main(void) {
    printf("Enter the number of strings to concatenate: ");
    char *count_line = read_line(stdin);
    if (count_line == NULL) {
        return 1;
    }

    int count = atoi(count_line);
    free(count_line);

    if (count <= 0) {
        fprintf(stderr, "Invalid count entered.\n");
        return 1;
    }

    char **strings = (char **)malloc((size_t)count * sizeof(char *));
    if (strings == NULL) {
        perror("Failed to allocate memory for string array");
        return 1;
    }

    for (int i = 0; i < count; i++) {
        printf("Enter string %d: ", i + 1);
        strings[i] = read_line(stdin);
        if (strings[i] == NULL) {
            strings[i] = (char *)malloc(1);
            if (strings[i] != NULL) {
                strings[i][0] = '\0';
            }
        }
    }

    char *result = concatenate_strings(strings, (size_t)count);
    if (result != NULL) {
        printf("\nConcatenated string:\n%s\n", result);
        free(result);
    } else {
        fprintf(stderr, "Failed to concatenate strings.\n");
    }

    for (int i = 0; i < count; i++) {
        free(strings[i]);
    }
    free(strings);

    return 0;
}