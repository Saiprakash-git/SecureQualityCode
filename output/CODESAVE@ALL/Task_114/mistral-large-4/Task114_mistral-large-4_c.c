#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

size_t string_length(const char *str) {
    size_t len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

void string_copy(char *dest, const char *src) {
    size_t i = 0;
    while (src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

void string_concat(char *dest, const char *src) {
    size_t dest_len = string_length(dest);
    size_t i = 0;
    while (src[i] != '\0') {
        dest[dest_len + i] = src[i];
        i++;
    }
    dest[dest_len + i] = '\0';
}

int string_compare(const char *str1, const char *str2) {
    size_t i = 0;
    while (str1[i] != '\0' && str2[i] != '\0') {
        if (str1[i] != str2[i]) {
            return str1[i] - str2[i];
        }
        i++;
    }
    return str1[i] - str2[i];
}

void string_reverse(char *str) {
    size_t len = string_length(str);
    size_t i;
    char temp;
    for (i = 0; i < len / 2; i++) {
        temp = str[i];
        str[i] = str[len - 1 - i];
        str[len - 1 - i] = temp;
    }
}

void string_to_upper(char *str) {
    size_t i = 0;
    while (str[i] != '\0') {
        if (str[i] >= 'a' && str[i] <= 'z') {
            str[i] = str[i] - 'a' + 'A';
        }
        i++;
    }
}

void string_to_lower(char *str) {
    size_t i = 0;
    while (str[i] != '\0') {
        if (str[i] >= 'A' && str[i] <= 'Z') {
            str[i] = str[i] - 'A' + 'a';
        }
        i++;
    }
}

int string_contains(const char *str, const char *substr) {
    size_t str_len = string_length(str);
    size_t sub_len = string_length(substr);
    size_t i, j;

    if (sub_len == 0) return 1;
    if (str_len < sub_len) return 0;

    for (i = 0; i <= str_len - sub_len; i++) {
        for (j = 0; j < sub_len; j++) {
            if (str[i + j] != substr[j]) {
                break;
            }
        }
        if (j == sub_len) {
            return 1;
        }
    }
    return 0;
}

void string_substring(const char *src, char *dest, size_t start, size_t length) {
    size_t src_len = string_length(src);
    size_t i;

    if (start >= src_len) {
        dest[0] = '\0';
        return;
    }

    if (start + length > src_len) {
        length = src_len - start;
    }

    for (i = 0; i < length; i++) {
        dest[i] = src[start + i];
    }
    dest[length] = '\0';
}

void string_trim(char *str) {
    size_t len = string_length(str);
    size_t start = 0;
    size_t end = len;
    size_t i;

    while (start < len && (str[start] == ' ' || str[start] == '\t' || str[start] == '\n')) {
        start++;
    }

    while (end > start && (str[end - 1] == ' ' || str[end - 1] == '\t' || str[end - 1] == '\n')) {
        end--;
    }

    for (i = 0; i < end - start; i++) {
        str[i] = str[start + i];
    }
    str[end - start] = '\0';
}

void string_replace(char *str, const char *old, const char *new_str) {
    size_t str_len = string_length(str);
    size_t old_len = string_length(old);
    size_t new_len = string_length(new_str);
    char buffer[1024];
    size_t i = 0, j = 0;

    while (i < str_len) {
        if (i + old_len <= str_len && string_compare(str + i, old) == 0) {
            size_t k;
            for (k = 0; k < new_len; k++) {
                buffer[j++] = new_str[k];
            }
            i += old_len;
        } else {
            buffer[j++] = str[i++];
        }
    }
    buffer[j] = '\0';
    string_copy(str, buffer);
}

int main() {
    char str1[256] = "Hello";
    char str2[256] = "World";
    char dest[512];
    char buffer[256];
    char sub[256];

    printf("Original strings:\n");
    printf("str1: '%s'\n", str1);
    printf("str2: '%s'\n", str2);

    printf("\nString length:\n");
    printf("Length of str1: %zu\n", string_length(str1));

    printf("\nString copy:\n");
    string_copy(dest, str1);
    printf("Copied str1 to dest: '%s'\n", dest);

    printf("\nString concatenation:\n");
    string_copy(dest, str1);
    string_concat(dest, " ");
    string_concat(dest, str2);
    printf("Concatenated: '%s'\n", dest);

    printf("\nString comparison:\n");
    printf("Compare '%s' and '%s': %d\n", str1, str2, string_compare(str1, str2));
    printf("Compare '%s' and '%s': %d\n", str1, "Hello", string_compare(str1, "Hello"));

    printf("\nString reversal:\n");
    string_copy(buffer, str1);
    string_reverse(buffer);
    printf("Reversed '%s': '%s'\n", str1, buffer);

    printf("\nCase conversion:\n");
    string_copy(buffer, "Hello World");
    string_to_upper(buffer);
    printf("Uppercase: '%s'\n", buffer);
    string_to_lower(buffer);
    printf("Lowercase: '%s'\n", buffer);

    printf("\nSubstring search:\n");
    printf("Does '%s' contain 'llo'? %s\n", str1, string_contains(str1, "llo") ? "Yes" : "No");
    printf("Does '%s' contain 'xyz'? %s\n", str1, string_contains(str1, "xyz") ? "Yes" : "No");

    printf("\nSubstring extraction:\n");
    string_substring("Hello World", sub, 6, 5);
    printf("Substring (6,5) of 'Hello World': '%s'\n", sub);

    printf("\nString trim:\n");
    string_copy(buffer, "   Hello World   ");
    printf("Before trim: '%s'\n", buffer);
    string_trim(buffer);
    printf("After trim: '%s'\n", buffer);

    printf("\nString replace:\n");
    string_copy(buffer, "Hello World, World is great");
    printf("Before replace: '%s'\n", buffer);
    string_replace(buffer, "World", "Universe");
    printf("After replace: '%s'\n", buffer);

    return 0;
}