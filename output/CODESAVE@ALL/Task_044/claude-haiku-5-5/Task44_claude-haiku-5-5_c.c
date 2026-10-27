```c
#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_INPUT 256
#define MAX_TOKENS 64

typedef enum {
    READ_OK = 0,
    READ_EOF = -1,
    READ_TOO_LONG = -2
} read_status;

typedef enum {
    OP_OK = 0,
    OP_INVALID_INPUT,
    OP_TOO_LONG,
    OP_OVERFLOW,
    OP_NOT_INTEGER
} op_status;

/* Reads one line safely into buf. Strips the trailing newline. */
static read_status read_line(char *buf, size_t size) {
    if (buf == NULL || size < 2) {
        return READ_EOF;
    }
    if (fgets(buf, (int)size, stdin) == NULL) {
        return READ_EOF;
    }

    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
        return READ_OK;
    }

    if (feof(stdin)) {
        return READ_OK;
    }

    /* Line was longer than the buffer: discard the remainder. */
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {
        /* discard */
    }
    buf[0] = '\0';
    return READ_TOO_LONG;
}

/* Accepts only printable ASCII characters (including space). */
static op_status validate_text(const char *s) {
    if (s == NULL) {
        return OP_INVALID_INPUT;
    }
    size_t len = strlen(s);
    if (len > MAX_INPUT) {
        return OP_TOO_LONG;
    }
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)s[i];
        if (!isprint(c)) {
            return OP_INVALID_INPUT;
        }
    }
    return OP_OK;
}

/* Parses a menu choice in the range [min, max]. */
static op_status parse_int_range(const char *s, int min, int max, int *out) {
    if (s == NULL || out == NULL) {
        return OP_INVALID_INPUT;
    }
    while (isspace((unsigned char)*s)) {
        s++;
    }
    if (*s == '\0') {
        return OP_INVALID_INPUT;
    }

    char *end = NULL;
    errno = 0;
    long value = strtol(s, &end, 10);
    if (errno == ERANGE || value < min || value > max) {
        return OP_OVERFLOW;
    }
    while (end && isspace((unsigned char)*end)) {
        end++;
    }
    if (end == NULL || *end != '\0') {
        return OP_NOT_INTEGER;
    }
    *out = (int)value;
    return OP_OK;
}

/* Reverses input into out. out_size must be at least strlen(in)+1. */
static op_status op_reverse(const char *in, char *out, size_t out_size) {
    if (in == NULL || out == NULL) {
        return OP_INVALID_INPUT;
    }
    size_t len = strlen(in);
    if (out_size < len + 1) {
        return OP_TOO_LONG;
    }
    for (size_t i = 0; i < len; i++) {
        out[i] = in[len - 1 - i];
    }
    out[len] = '\0';
    return OP_OK;
}

/* Converts input to uppercase into out. */
static op_status op_uppercase(const char *in, char *out, size_t out_size) {
    if (in == NULL || out == NULL) {
        return OP_INVALID_INPUT;
    }
    size_t len = strlen(in);
    if (out_size < len + 1) {
        return OP_TOO_LONG;
    }
    for (size_t i = 0; i < len; i++) {
        out[i] = (char)toupper((unsigned char)in[i]);
    }
    out[len] = '\0';
    return OP_OK;
}

/* Counts vowels (a, e, i, o, u; case-insensitive). */
static op_status op_count_vowels(const char *in, int *count) {
    if (in == NULL || count == NULL) {
        return OP_INVALID_INPUT;
    }
    int total = 0;
    for (const char *p = in; *p != '\0'; p++) {
        switch (tolower((unsigned char)*p)) {
            case 'a': case 'e': case 'i': case 'o': case 'u':
                total++;
                break;
            default:
                break;
        }
    }
    *count = total;
    return OP_OK;
}

/* Counts whitespace-separated words. */
static op_status op_word_count(const char *in, int *count) {
    if (in == NULL || count == NULL) {
        return OP_INVALID_INPUT;
    }
    int words = 0;
    bool in_word = false;
    for (const char *p = in; *p != '\0'; p++) {
        if (isspace((unsigned char)*p)) {
            in_word = false;
        } else if (!in_word) {
            in_word = true;
            words++;
        }
    }
    *count = words;
    return OP_OK;
}

/* Checks palindrome ignoring non-alphanumeric characters and case. */
static op_status op_is_palindrome(const char *in, bool *result) {
    if (in == NULL || result == NULL) {
        return OP_INVALID_INPUT;
    }
    size_t i = 0;
    size_t j = strlen(in);
    bool ok = true;

    while (ok && j > 0) {
        while (i < j && !isalnum((unsigned char)in[i])) {
            i++;
        }
        while (j > i && !isalnum((unsigned char)in[j - 1])) {
            j--;
        }
        if (i >= j) {
            break;
        }
        if (tolower((unsigned char)in[i]) != tolower((unsigned char)in[j - 1])) {
            ok = false;
        }
        i++;
        j--;
    }
    *result = ok;
    return OP_OK;
}

/* Sums all whitespace-separated integer tokens. Every token must be an integer. */
static op_status op_sum_integers(const char *in, long *sum) {
    if (in == NULL || sum == NULL) {
        return OP_INVALID_INPUT;
    }

    char copy[MAX_INPUT + 1];
    size_t len = strlen(in);
    if (len > MAX_INPUT) {
        return OP_TOO_LONG;
    }
    memcpy(copy, in, len + 1);

    long total = 0;
    int tokens = 0;
    char *saveptr = NULL;
    char *tok = strtok_r(copy, " \t", &saveptr);

    while (tok != NULL) {
        if (++tokens > MAX_TOKENS) {
            return OP_TOO_LONG;
        }
        char *end = NULL;
        errno = 0;
        long v = strtol(tok, &end, 10);
        if (end == tok || *end != '\0') {
            return OP_NOT_INTEGER;
        }
        if (errno == ERANGE) {
            return OP_OVERFLOW;
        }
        if ((v > 0 && total > LONG_MAX - v) || (v < 0 && total < LONG_MIN - v)) {
            return OP_OVERFLOW;
        }
        total += v;
        tok = strtok_r(NULL, " \t", &saveptr);
    }

    if (tokens == 0) {
        return OP_INVALID_INPUT;
    }
    *sum = total;
    return OP_OK;
}

static void print_menu(void) {
    printf("\n=== Text Operations ===\n");
    printf("1. Reverse text\n");
    printf("2. Convert to uppercase\n");
    printf("3. Count vowels\n");
    printf("4. Count words\n");
    printf("5. Check palindrome\n");
    printf("6. Sum integers in text\n");
    printf("7. Exit\n");
    printf("Choose an option (1-7): ");
}

static const char *status_message(op_status s) {
    switch (s) {
        case OP_OK:           return "OK";
        case OP_INVALID_INPUT:return "Invalid input (only printable ASCII allowed)";
        case OP_TOO_LONG:    return "Input too long";
        case OP_OVERFLOW:    return "Numeric value out of range";
        case OP_NOT_INTEGER: