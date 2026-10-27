#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int read_password_from_file(const char *filename, char *buffer, int buffer_size) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        return 0;
    }
    if (fgets(buffer, buffer_size, file) == NULL) {
        fclose(file);
        return 0;
    }
    fclose(file);
    buffer[strcspn(buffer, "\n")] = '\0';
    return 1;
}

void get_user_password(char *buffer, int buffer_size) {
    printf("Enter password: ");
    fgets(buffer, buffer_size, stdin);
    buffer[strcspn(buffer, "\n")] = '\0';
}

int compare_passwords(const char *stored, const char *entered) {
    int i = 0;
    while (stored[i] != '\0' && entered[i] != '\0') {
        if (stored[i] != entered[i]) {
            return 0;
        }
        i++;
    }
    if (stored[i] == '\0' && entered[i] == '\0') {
        return 1;
    }
    return 0;
}

int main() {
    char stored_password[256];
    char user_password[256];
    const char *filename = "password.txt";

    if (!read_password_from_file(filename, stored_password, sizeof(stored_password))) {
        printf("Error reading password file.\n");
        return 1;
    }

    get_user_password(user_password, sizeof(user_password));

    if (compare_passwords(stored_password, user_password)) {
        printf("Password correct.\n");
        return 0;
    } else {
        printf("Password incorrect.\n");
        return 1;
    }
}