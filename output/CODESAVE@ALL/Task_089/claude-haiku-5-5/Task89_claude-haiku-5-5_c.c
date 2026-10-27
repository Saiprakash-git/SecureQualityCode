#include <stdio.h>
#include <string.h>

#define MAX_PASS 256
#define PASS_FILE "password.txt"

void strip_newline(char *s) {
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r')) {
        s[--n] = '\0';
    }
}

int read_stored_password(const char *filename, char *buffer, size_t size) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        return -1;
    }
    if (fgets(buffer, (int)size, fp) == NULL) {
        buffer[0] = '\0';
        fclose(fp);
        return -1;
    }
    fclose(fp);
    strip_newline(buffer);
    return 0;
}

int read_user_password(char *buffer, size_t size) {
    if (fgets(buffer, (int)size, stdin) == NULL) {
        buffer[0] = '\0';
        return -1;
    }
    strip_newline(buffer);
    return 0;
}

int compare_passwords(const char *stored, const char *entered) {
    size_t i = 0;
    while (stored[i] != '\0' && entered[i] != '\0') {
        if (stored[i] != entered[i]) {
            return 0;
        }
        i++;
    }
    return stored[i] == entered[i];
}

int main(void) {
    char stored[MAX_PASS];
    char entered[MAX_PASS];

    if (read_stored_password(PASS_FILE, stored, sizeof(stored)) != 0) {
        fprintf(stderr, "Error: could not read password from '%s'.\n", PASS_FILE);
        return 1;
    }

    printf("Enter password: ");
    fflush(stdout);

    if (read_user_password(entered, sizeof(entered)) != 0) {
        fprintf(stderr, "Error: could not read input.\n");
        return 1;
    }

    if (compare_passwords(stored, entered)) {
        printf("Password accepted.\n");
        return 0;
    } else {
        printf("Password incorrect.\n");
        return 2;
    }
}