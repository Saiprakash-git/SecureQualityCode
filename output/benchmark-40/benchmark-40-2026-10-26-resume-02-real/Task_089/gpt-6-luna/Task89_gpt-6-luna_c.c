#include <stdio.h>
#include <stdlib.h>

static char *read_line(FILE *stream)
{
    size_t capacity = 64;
    size_t length = 0;
    char *line = malloc(capacity);
    int ch;

    if (line == NULL)
        return NULL;

    while ((ch = fgetc(stream)) != EOF && ch != '\n') {
        if (length + 1 >= capacity) {
            size_t new_capacity = capacity * 2;
            char *resized = realloc(line, new_capacity);

            if (resized == NULL) {
                free(line);
                return NULL;
            }

            line = resized;
            capacity = new_capacity;
        }

        line[length++] = (char)ch;
    }

    if (ch == EOF && (ferror(stream) || length == 0)) {
        free(line);
        return NULL;
    }

    if (length > 0 && line[length - 1] == '\r')
        length--;

    line[length] = '\0';
    return line;
}

static int passwords_match(const char *stored, const char *entered)
{
    size_t i = 0;

    while (stored[i] != '\0' && entered[i] != '\0') {
        if (stored[i] != entered[i])
            return 0;
        i++;
    }

    return stored[i] == '\0' && entered[i] == '\0';
}

int main(int argc, char *argv[])
{
    FILE *file;
    char *stored_password;
    char *entered_password;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <password-file>\n", argv[0]);
        return 2;
    }

    file = fopen(argv[1], "r");
    if (file == NULL) {
        perror("Could not open password file");
        return 2;
    }

    stored_password = read_line(file);
    fclose(file);

    if (stored_password == NULL) {
        fprintf(stderr, "Could not read a password from the file.\n");
        return 2;
    }

    printf("Password: ");
    fflush(stdout);

    entered_password = read_line(stdin);
    if (entered_password == NULL) {
        fprintf(stderr, "Could not read the entered password.\n");
        free(stored_password);
        return 2;
    }

    if (passwords_match(stored_password, entered_password))
        puts("Password matches.");
    else
        puts("Password does not match.");

    free(stored_password);
    free(entered_password);
    return 0;
}