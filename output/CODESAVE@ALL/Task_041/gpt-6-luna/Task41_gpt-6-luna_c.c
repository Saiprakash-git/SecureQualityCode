#include <stdio.h>
#include <string.h>

#define INPUT_CAPACITY 1024

static int read_line(char *buffer, size_t capacity)
{
    size_t length;
    int ch;

    if (buffer == NULL || capacity < 2) {
        return -1;
    }

    if (fgets(buffer, (int)capacity, stdin) == NULL) {
        return feof(stdin) ? 0 : -1;
    }

    length = strlen(buffer);

    if (length > 0 && buffer[length - 1] == '\n') {
        buffer[length - 1] = '\0';
        return 1;
    }

    ch = fgetc(stdin);
    if (ch == EOF) {
        return ferror(stdin) ? -1 : 1;
    }

    if (ch == '\n') {
        return 1;
    }

    while (ch != '\n' && ch != EOF) {
        ch = fgetc(stdin);
    }

    return -1;
}

static size_t process_string(const char *input)
{
    return strlen(input);
}

int main(int argc, char *argv[])
{
    char input[INPUT_CAPACITY];
    int result;
    size_t length;

    (void)argc;
    (void)argv;

    printf("Enter a string: ");
    fflush(stdout);

    result = read_line(input, sizeof input);
    if (result == 0) {
        return 0;
    }
    if (result < 0) {
        fprintf(stderr, "Input is too long or could not be read.\n");
        return 1;
    }

    length = process_string(input);
    printf("You entered: %s\n", input);
    printf("Length: %zu characters\n", length);

    return 0;
}