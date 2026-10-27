#include <stdio.h>
#include <string.h>
#include <limits.h>

enum ReadResult {
    READ_ERROR = -2,
    READ_TOO_LONG = -1,
    READ_EOF = 0,
    READ_OK = 1
};

static enum ReadResult read_line(FILE *input, char *buffer, size_t capacity)
{
    char *newline;
    int ch;

    if (input == NULL || buffer == NULL || capacity < 2 || capacity > INT_MAX)
        return READ_ERROR;

    if (fgets(buffer, (int)capacity, input) == NULL)
        return ferror(input) ? READ_ERROR : READ_EOF;

    newline = strchr(buffer, '\n');
    if (newline != NULL) {
        *newline = '\0';
    } else {
        ch = fgetc(input);

        if (ch == EOF) {
            if (ferror(input))
                return READ_ERROR;
        } else if (ch != '\n') {
            while (ch != '\n' && ch != EOF)
                ch = fgetc(input);

            return ferror(input) ? READ_ERROR : READ_TOO_LONG;
        }
    }

    {
        size_t length = strlen(buffer);
        if (length > 0 && buffer[length - 1] == '\r')
            buffer[length - 1] = '\0';
    }

    return READ_OK;
}

int main(void)
{
    char buffer[256];
    enum ReadResult result;

    printf("Enter text (up to %zu characters): ", sizeof(buffer) - 1);
    fflush(stdout);

    result = read_line(stdin, buffer, sizeof(buffer));

    if (result == READ_OK) {
        printf("You entered: %s\n", buffer);
        return 0;
    }

    if (result == READ_TOO_LONG) {
        fprintf(stderr, "Input is too long.\n");
        return 1;
    }

    if (result == READ_EOF) {
        fprintf(stderr, "No input received.\n");
        return 1;
    }

    fprintf(stderr, "Error reading input.\n");
    return 1;
}