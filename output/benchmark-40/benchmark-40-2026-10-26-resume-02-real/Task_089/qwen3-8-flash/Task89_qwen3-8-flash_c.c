#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PASSWORD 256

static char *trim_newline(char *password) {
    size_t length = strlen(password);

    while (length > 0 &&
           (password[length - 1] == '\n' || password[length - 1] == '\r')) {
        password[length - 1] = '\0';
        --length;
    }

    return password;
}

static int read_password_from_file(const char *filename,
                                   char *buffer,
                                   size_t buffer_size) {
    FILE *file = fopen(filename, "r");

    if (file == NULL) {
        return 0;
    }

    if (fgets(buffer, (int)buffer_size, file) == NULL) {
        fclose(file);
        return 0;
    }

    fclose(file);
    trim_newline(buffer);

    return 1;
}

static int read_password_from_stdin(char *buffer, size_t buffer_size) {
    printf("Enter password: ");
    fflush(stdout);

    if (fgets(buffer, (int)buffer_size, stdin) == NULL) {
        return 0;
    }

    trim_newline(buffer);

    return 1;
}

static int passwords_match(const char *stored_password,
                           const char *entered_password) {
    size_t index = 0;

    while (1) {
        char stored_char = stored_password[index];
        char entered_char = entered_password[index];

        if (stored_char != entered_char) {
            return 0;
        }

        if (stored_char == '\0') {
            return 1;
        }

        ++index;
    }
}

int main(int argc, char *argv[]) {
    const char *filename = (argc > 1) ? argv[1] : "password.txt";

    char stored_password[MAX_PASSWORD];
    char entered_password[MAX_PASSWORD];

    if (!read_password_from_file(filename,
                                 stored_password,
                                 sizeof(stored_password))) {
        fprintf(stderr, "Could not read stored password from file.\n");
        return 1;
    }

    if (!read_password_from_stdin(entered_password,
                                  sizeof(entered_password))) {
        fprintf(stderr, "Could not read entered password.\n");
        return 1;
    }

    if (passwords_match(stored_password, entered_password)) {
        printf("Password matches.\n");
        return 0;
    }

    printf("Password does not match.\n");
    return 1;
}