#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PASSWORD_LEN 256
#define PASSWORD_FILE "password.txt"

void strip_newline(char *str) {
    if (str == NULL) {
        return;
    }
    size_t len = strlen(str);
    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[len - 1] = '\0';
        len--;
    }
}

int read_password_from_file(const char *filename, char *buffer, size_t max_size) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        return 0;
    }

    if (fgets(buffer, (int)max_size, file) == NULL) {
        fclose(file);
        return 0;
    }

    fclose(file);
    strip_newline(buffer);
    return 1;
}

int compare_passwords_sequential(const char *stored, const char *entered) {
    size_t i = 0;

    if (stored == NULL || entered == NULL) {
        return 0;
    }

    while (stored[i] != '\0' && entered[i] != '\0') {
        if (stored[i] != entered[i]) {
            return 0;
        }
        i++;
    }

    return (stored[i] == '\0' && entered[i] == '\0') ? 1 : 0;
}

int ensure_file_exists(const char *filename, const char *default_password) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        file = fopen(filename, "w");
        if (file == NULL) {
            return 0;
        }
        fputs(default_password, file);
        fputc('\n', file);
        fclose(file);
    } else {
        fclose(file);
    }
    return 1;
}

int main(void) {
    char stored_password[MAX_PASSWORD_LEN];
    char entered_password[MAX_PASSWORD_LEN];

    if (!ensure_file_exists(PASSWORD_FILE, "DefaultSecret123")) {
        fprintf(stderr, "Error: Unable to access or initialize %s\n", PASSWORD_FILE);
        return EXIT_FAILURE;
    }

    if (!read_password_from_file(PASSWORD_FILE, stored_password, sizeof(stored_password))) {
        fprintf(stderr, "Error: Failed to read password from file %s\n", PASSWORD_FILE);
        return EXIT_FAILURE;
    }

    printf("Enter password: ");
    if (fgets(entered_password, sizeof(entered_password), stdin) == NULL) {
        fprintf(stderr, "Error reading user input\n");
        return EXIT_FAILURE;
    }
    strip_newline(entered_password);

    int is_match = compare_passwords_sequential(stored_password, entered_password);

    if (is_match) {
        printf("Access granted: Passwords match.\n");
    } else {
        printf("Access denied: Password mismatch.\n");
    }

    return is_match ? EXIT_SUCCESS : EXIT_FAILURE;
}