#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MIN_ADMIN_PASSWORD_LEN 12
#define MAX_ADMIN_PASSWORD_LEN 128

static int contains_case_insensitive(const char *haystack, const char *needle) {
    if (!haystack || !needle) {
        return 0;
    }

    size_t nlen = strlen(needle);
    size_t hlen = strlen(haystack);

    if (nlen > hlen) {
        return 0;
    }

    for (size_t i = 0; i <= hlen - nlen; i++) {
        size_t j = 0;
        while (j < nlen && tolower((unsigned char)haystack[i + j]) == tolower((unsigned char)needle[j])) {
            j++;
        }
        if (j == nlen) {
            return 1;
        }
    }
    return 0;
}

int is_valid_admin_password(const char *password) {
    if (password == NULL) {
        return 0;
    }

    size_t len = strlen(password);
    if (len < MIN_ADMIN_PASSWORD_LEN || len > MAX_ADMIN_PASSWORD_LEN) {
        return 0;
    }

    int has_upper = 0;
    int has_lower = 0;
    int has_digit = 0;
    int has_special = 0;

    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)password[i];
        if (isspace(c)) {
            return 0;
        }
        if (isupper(c)) {
            has_upper = 1;
        } else if (islower(c)) {
            has_lower = 1;
        } else if (isdigit(c)) {
            has_digit = 1;
        } else if (ispunct(c)) {
            has_special = 1;
        }
    }

    if (!has_upper || !has_lower || !has_digit || !has_special) {
        return 0;
    }

    if (contains_case_insensitive(password, "admin") ||
        contains_case_insensitive(password, "password") ||
        contains_case_insensitive(password, "root")) {
        return 0;
    }

    return 1;
}

int main(int argc, char *argv[]) {
    char input[256];
    const char *pwd_to_check = NULL;

    if (argc > 1) {
        pwd_to_check = argv[1];
    } else {
        printf("Enter password to validate: ");
        if (fgets(input, sizeof(input), stdin) == NULL) {
            return 1;
        }
        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') {
            input[len - 1] = '\0';
        }
        pwd_to_check = input;
    }

    if (is_valid_admin_password(pwd_to_check)) {
        printf("VALID: Password meets admin security criteria.\n");
        return 0;
    } else {
        printf("INVALID: Password does not meet admin security criteria.\n");
        printf("Requirements:\n");
        printf(" - Minimum %d characters\n", MIN_ADMIN_PASSWORD_LEN);
        printf(" - At least one uppercase letter (A-Z)\n");
        printf(" - At least one lowercase letter (a-z)\n");
        printf(" - At least one digit (0-9)\n");
        printf(" - At least one special character (e.g. !@#$%%^&*)\n");
        printf(" - No whitespace allowed\n");
        printf(" - Cannot contain 'admin', 'password', or 'root'\n");
        return 1;
    }
}