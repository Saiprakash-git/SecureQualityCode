#include <stdio.h>
#include <string.h>
#include <ctype.h>

int validate_admin_password(const char *password) {
    if (password == NULL) {
        return 0;
    }

    size_t len = strlen(password);
    if (len < 12) {
        return 0;
    }

    int has_upper = 0;
    int has_lower = 0;
    int has_digit = 0;
    int has_special = 0;

    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)password[i];
        if (isupper(c)) {
            has_upper = 1;
        } else if (islower(c)) {
            has_lower = 1;
        } else if (isdigit(c)) {
            has_digit = 1;
        } else if (ispunct(c) || isspace(c)) {
            has_special = 1;
        }
    }

    return has_upper && has_lower && has_digit && has_special;
}

int main(int argc, char *argv[]) {
    char password[256];

    if (argc > 1) {
        strncpy(password, argv[1], sizeof(password) - 1);
        password[sizeof(password) - 1] = '\0';
    } else {
        printf("Enter admin password: ");
        if (fgets(password, sizeof(password), stdin) == NULL) {
            fprintf(stderr, "Error reading input\n");
            return 1;
        }
        password[strcspn(password, "\n")] = '\0';
    }

    if (validate_admin_password(password)) {
        printf("VALID\n");
        return 0;
    } else {
        printf("INVALID\n");
        return 1;
    }
}