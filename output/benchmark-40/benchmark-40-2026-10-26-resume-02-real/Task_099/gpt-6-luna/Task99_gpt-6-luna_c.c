#define _POSIX_C_SOURCE 200809L

#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s FILE\n", argv[0]);
        return EXIT_FAILURE;
    }

    FILE *file = fopen(argv[1], "r");
    if (file == NULL) {
        perror(argv[1]);
        return EXIT_FAILURE;
    }

    regex_t pattern;
    int result = regcomp(&pattern, "^[0-9]+.*\\.$", REG_EXTENDED);
    if (result != 0) {
        char error[256];
        regerror(result, &pattern, error, sizeof(error));
        fprintf(stderr, "Regular expression error: %s\n", error);
        fclose(file);
        return EXIT_FAILURE;
    }

    char *line = NULL;
    size_t capacity = 0;
    ssize_t length;
    int status = EXIT_SUCCESS;

    while ((length = getline(&line, &capacity, file)) != -1) {
        while (length > 0 && (line[length - 1] == '\n' || line[length - 1] == '\r')) {
            line[--length] = '\0';
        }

        if (regexec(&pattern, line, 0, NULL, 0) == 0) {
            if (fwrite(line, 1, (size_t)length, stdout) != (size_t)length ||
                putchar('\n') == EOF) {
                status = EXIT_FAILURE;
                break;
            }
        }
    }

    if (ferror(file)) {
        perror(argv[1]);
        status = EXIT_FAILURE;
    }

    free(line);
    regfree(&pattern);
    fclose(file);
    return status;
}