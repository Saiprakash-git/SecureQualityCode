#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>

static bool is_valid_admin_password(const char *password)
{
    size_t len;
    size_t i;
    bool has_upper;
    bool has_lower;
    bool has_digit;
    bool has_special;

    if (password == NULL) {
        return false;
    }

    len = strlen(password);

    if (len < 12 || len > 64) {
        return false;
    }

    has_upper = false;
    has_lower = false;
    has_digit = false;
    has_special = false;

    for (i = 0; i < len; ++i) {
        unsigned char ch = (unsigned char)password[i];

        if (isspace(ch)) {
            return false;
        }

        if (isupper(ch)) {
            has_upper = true;
        } else if (islower(ch)) {
            has_lower = true;
        } else if (isdigit(ch)) {
            has_digit = true;
        } else if (isprint(ch) && !isalnum(ch)) {
            has_special = true;
        } else {
            return false;
        }
    }

    return has_upper && has_lower && has_digit && has_special;
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        return 2;
    }

    return is_valid_admin_password(argv[1]) ? 0 : 1;
}