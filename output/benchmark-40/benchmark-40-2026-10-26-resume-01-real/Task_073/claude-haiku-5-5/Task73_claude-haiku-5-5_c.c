#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#define ADMIN_MIN_LENGTH 12
#define ADMIN_MAX_LENGTH 128

typedef enum {
    PWD_VALID = 0,
    PWD_NULL,
    PWD_TOO_SHORT,
    PWD_TOO_LONG,
    PWD_NO_UPPER,
    PWD_NO_LOWER,
    PWD_NO_DIGIT,
    PWD_NO_SPECIAL,
    PWD_HAS_SPACE,
    PWD_CONTAINS_USERNAME
} PasswordResult;

static bool is_special_char(char c) {
    return ispunct((unsigned char)c) != 0;
}

static bool contains_ci(const char *haystack, const char *needle) {
    size_t hlen = strlen(haystack);
    size_t nlen = strlen(needle);
    if (nlen == 0 || nlen > hlen) {
        return false;
    }
    for (size_t i = 0; i + nlen <= hlen; i++) {
        size_t j = 0;
        while (j < nlen &&
               tolower((unsigned char)haystack[i + j]) == tolower((unsigned char)needle[j])) {
            j++;
        }
        if (j == nlen) {
            return true;
        }
    }
    return false;
}

PasswordResult check_admin_password(const char *password, const char *username) {
    if (password == NULL) {
        return PWD_NULL;
    }

    size_t len = strlen(password);
    if (len < ADMIN_MIN_LENGTH) {
        return PWD_TOO_SHORT;
    }
    if (len > ADMIN_MAX_LENGTH) {
        return PWD_TOO_LONG;
    }

    bool has_upper = false;
    bool has_lower = false;
    bool has_digit = false;
    bool has_special = false;
    bool has_space = false;

    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)password[i];
        if (isupper(c)) {
            has_upper = true;
        } else if (islower(c)) {
            has_lower = true;
        } else if (isdigit(c)) {
            has_digit = true;
        } else if (isspace(c)) {
            has_space = true;
        } else if (is_special_char((char)c)) {
            has_special = true;
        }
    }

    if (has_space) {
        return PWD_HAS_SPACE;
    }
    if (!has_upper) {
        return PWD_NO_UPPER;
    }
    if (!has_lower) {
        return PWD_NO_LOWER;
    }
    if (!has_digit) {
        return PWD_NO_DIGIT;
    }
    if (!has_special) {
        return PWD_NO_SPECIAL;
    }
    if (username != NULL && strlen(username) > 0 && contains_ci(password, username)) {
        return PWD_CONTAINS_USERNAME;
    }

    return PWD_VALID;
}

const char *password_result_message(PasswordResult result) {
    switch (result) {
        case PWD_VALID:              return "Password is valid for admin user.";
        case PWD_NULL:               return "Password is missing.";
        case PWD_TOO_SHORT:          return "Password must be at least 12 characters long.";
        case PWD_TOO_LONG:           return "Password must not exceed 128 characters.";
        case PWD_NO_UPPER:           return "Password must contain at least one uppercase letter.";
        case PWD_NO_LOWER:           return "Password must contain at least one lowercase letter.";
        case PWD_NO_DIGIT:           return "Password must contain at least one digit.";
        case PWD_NO_SPECIAL:         return "Password must contain at least one special character.";
        case PWD_HAS_SPACE:          return "Password must not contain whitespace.";
        case PWD_CONTAINS_USERNAME:  return "Password must not contain the username.";
        default:                     return "Unknown error.";
    }
}

static void strip_newline(char *s) {
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r')) {
        s[--len] = '\0';
    }
}

int main(void) {
    char username[64];
    char password[ADMIN_MAX_LENGTH + 2];

    printf("Enter admin username: ");
    if (fgets(username, sizeof(username), stdin) == NULL) {
        fprintf(stderr, "Failed to read username.\n");
        return 1;
    }
    strip_newline(username);

    printf("Enter admin password: ");
    if (fgets(password, sizeof(password), stdin) == NULL) {
        fprintf(stderr, "Failed to read password.\n");
        return 1;
    }
    strip_newline(password);

    PasswordResult result = check_admin_password(password, username);
    printf("%s\n", password_result_message(result));

    return (result == PWD_VALID) ? 0 : 2;
}