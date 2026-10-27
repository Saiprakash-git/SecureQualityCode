#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 32

typedef enum {
    INPUT_SUCCESS,
    INPUT_TRUNCATED,
    INPUT_EMPTY,
    INPUT_EOF_OR_ERROR
} InputStatus;

InputStatus read_line(char *dest, size_t dest_size, FILE *stream) {
    if (dest == NULL || dest_size < 2 || stream == NULL) {
        return INPUT_EOF_OR_ERROR;
    }

    if (fgets(dest, (int)dest_size, stream) == NULL) {
        dest[0] = '\0';
        return INPUT_EOF_OR_ERROR;
    }

    size_t len = strlen(dest);

    if (len > 0 && dest[len - 1] == '\n') {
        dest[len - 1] = '\0';
        if (len == 1) {
            return INPUT_EMPTY;
        }
        return INPUT_SUCCESS;
    }

    /* Input exceeded buffer size; flush remainder of the line */
    int ch;
    while ((ch = fgetc(stream)) != '\n' && ch != EOF) {
        /* Discard extra characters */
    }

    return INPUT_TRUNCATED;
}

int main(void) {
    char buffer[BUFFER_SIZE];
    InputStatus status;

    printf("Buffer capacity: %d characters (including null terminator).\n", BUFFER_SIZE);
    printf("Enter input: ");
    fflush(stdout);

    status = read_line(buffer, sizeof(buffer), stdin);

    switch (status) {
        case INPUT_SUCCESS:
            printf("Result [SUCCESS]: \"%s\"\n", buffer);
            break;
        case INPUT_TRUNCATED:
            printf("Result [TRUNCATED]: \"%s\"\n", buffer);
            printf("Warning: Input exceeded buffer size and was safely truncated.\n");
            break;
        case INPUT_EMPTY:
            printf("Result [EMPTY]: No characters entered.\n");
            break;
        case INPUT_EOF_OR_ERROR:
            printf("Result [ERROR]: Reached EOF or encounter an error.\n");
            break;
    }

    return 0;
}