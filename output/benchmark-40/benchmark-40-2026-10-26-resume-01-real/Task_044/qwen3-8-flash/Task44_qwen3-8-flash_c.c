#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#define MAX_INPUT 256
#define MAX_IDENTIFIER 64

static int read_line(char *buffer, size_t size)
{
    size_t len;

    if (buffer == NULL || size == 0) {
        return 0;
    }

    if (fgets(buffer, (int)size, stdin) == NULL) {
        return 0;
    }

    len = strlen(buffer);

    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
        return 1;
    }

    if (len == size - 1) {
        return 0;
    }

    return 1;
}

static int parse_unsigned_decimal(const char *text, unsigned long long *value)
{
    unsigned long long result = 0;

    if (text == NULL || value == NULL || *text == '\0') {
        return 0;
    }

    for (; *text != '\0'; ++text) {
        unsigned long long digit;

        if (!isdigit((unsigned char)*text)) {
            return 0;
        }

        digit = (unsigned long long)(*text - '0');

        if (result > (ULLONG_MAX - digit) / 10ULL) {
            return 0;
        }

        result = result * 10ULL + digit;
    }

    *value = result;
    return 1;
}

static int validate_identifier(const char *text)
{
    size_t len;
    size_t i;

    if (text == NULL) {
        return 0;
    }

    len = strlen(text);

    if (len == 0 || len > 63) {
        return 0;
    }

    if (!(isalpha((unsigned char)text[0]) || text[0] == '_')) {
        return 0;
    }

    for (i = 1; i < len; ++i) {
        if (!(isalnum((unsigned char)text[i]) || text[i] == '_')) {
            return 0;
        }
    }

    return 1;
}

static int reverse_identifier(const char *text, char *out, size_t out_size)
{
    size_t len;
    size_t i;

    if (text == NULL || out == NULL || out_size == 0) {
        return 0;
    }

    len = strlen(text);

    if (len + 1 > out_size) {
        return 0;
    }

    for (i = 0; i < len; ++i) {
        out[i] = text[len - 1 - i];
    }

    out[len] = '\0';
    return 1;
}

static size_t count_vowels(const char *text)
{
    size_t count = 0;

    if (text == NULL) {
        return 0;
    }

    for (; *text != '\0'; ++text) {
        char c = (char)tolower((unsigned char)*text);

        if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u') {
            ++count;
        }
    }

    return count;
}

int main(void)
{
    char line[MAX_INPUT];
    char identifier[MAX_IDENTIFIER + 2];
    char reversed[MAX_IDENTIFIER];
    unsigned long long number;
    unsigned long long operation;

    printf("Enter a decimal integer between 1 and 46340: ");

    if (!read_line(line, sizeof(line))) {
        fprintf(stderr, "Invalid input.\n");
        return 1;
    }

    if (!parse_unsigned_decimal(line, &number) || number < 1 || number > 46340ULL) {
        fprintf(stderr, "Invalid integer.\n");
        return 1;
    }

    printf("Enter an identifier (letters, digits, underscore, max 63): ");

    if (!read_line(identifier, sizeof(identifier))) {
        fprintf(stderr, "Invalid identifier.\n");
        return 1;
    }

    if (!validate_identifier(identifier)) {
        fprintf(stderr, "Invalid identifier.\n");
        return 1;
    }

    printf("Enter operation (1 square, 2 reverse, 3 vowel count): ");

    if (!read_line(line, sizeof(line))) {
        fprintf(stderr, "Invalid operation.\n");
        return 1;
    }

    if (!parse_unsigned_decimal(line, &operation) || operation < 1 || operation > 3) {
        fprintf(stderr, "Invalid operation.\n");
        return