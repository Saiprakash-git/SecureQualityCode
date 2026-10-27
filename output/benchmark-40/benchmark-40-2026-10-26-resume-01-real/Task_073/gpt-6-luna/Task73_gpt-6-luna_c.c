#include <ctype.h>
#include <stdio.h>
#include <string.h>

int is_valid_admin_password(const char *password)
{
    int has_upper = 0;
    int has_lower = 0;
    int has_digit = 0;
    int has_special = 0;
    size_t length;

    if (password == NULL)
        return 0;

    length = strlen(password);
    if (length < 8)
        return 0;

    for (size_t i = 0; i < length; i++) {
        unsigned char ch = (unsigned char)password[i];

        if (isupper(ch))
            has_upper = 1;
        else if (islower(ch))
            has_lower = 1;
        else if (isdigit(ch))
            has_digit = 1;
        else
            has_special = 1;
    }

    return has_upper && has_lower && has_digit && has_special;
}

int main(int argc, char *argv[])
{
    char password[1024];

    if (argc > 1) {
        printf("%s\n", is_valid_admin_password(argv[1]) ? "Valid" : "Invalid");
        return 0;
    }

    if (fgets(password, sizeof(password), stdin) == NULL)
        return 1;

    password[strcspn(password, "\r\n")] = '\0';
    printf("%s\n", is_valid_admin_password(password) ? "Valid" : "Invalid");

    return 0;
}