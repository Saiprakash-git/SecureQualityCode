#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static size_t safe_strlen(const char *s) {
    return s ? strlen(s) : 0;
}

static int digit_count(int n) {
    int count = 0;

    if (n == 0) {
        return 1;
    }

    while (n > 0) {
        ++count;
        n /= 10;
    }

    return count;
}

char *likes(char **names, int count) {
    if (count < 0) {
        count = 0;
    }

    if (count == 0) {
        const char *text = "no one likes this";
        char *out = malloc(strlen(text) + 1);

        if (out) {
            memcpy(out, text, strlen(text) + 1);
        }

        return out;
    }

    const char *first = (names && names[0]) ? names[0] : "";
    const char *second = "";
    const char *third = "";

    if (count > 1 && names && names[1]) {
        second = names[1];
    }

    if (count > 2 && names && names[2]) {
        third = names[2];
    }

    size_t size = 0;

    if (count == 1) {
        size = safe_strlen(first) + 12;
    } else if (count == 2) {
        size = safe_strlen(first) + safe_strlen(second) + 5 + 10 + 1;
    } else if (count == 3) {
        size = safe_strlen(first) + safe_strlen(second) + safe_strlen(third) + 2 + 5 + 10 + 1;
    } else {
        size = safe_strlen(first) + safe_strlen(second) + 2 + 5 + (size_t)digit_count(count - 2) + 17 + 1;
    }

    char *out = malloc(size);

    if (!out) {
        return NULL;
    }

    if (count == 1) {
        snprintf(out, size, "%s likes this", first);
    } else if (count == 2) {
        snprintf(out, size, "%s and %s like this", first, second);
    } else if (count == 3) {
        snprintf(out, size, "%s, %s and %s like this", first, second, third);
    } else {
        snprintf(out, size, "%s, %s and %d others like this", first, second, count - 2);
    }

    return out;
}