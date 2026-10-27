#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stddef.h>

size_t string_length(const char *str) {
    size_t len = 0;
    if (!str) return 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

char *string_copy(char *dest, const char *src) {
    if (!dest || !src) return dest;
    char *ptr = dest;
    while (*src != '\0') {
        *ptr++ = *src++;
    }
    *ptr = '\0';
    return dest;
}

char *string_concat(char *dest, const char *src) {
    if (!dest || !src) return dest;
    char *ptr = dest;
    while (*ptr != '\0') {
        ptr++;
    }
    while (*src != '\0') {
        *ptr++ = *src++;
    }
    *ptr = '\0';
    return dest;
}

int string_compare(const char *s1, const char *s2) {
    if (!s1 && !s2) return 0;
    if (!s1) return -1;
    if (!s2) return 1;

    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

char *string_reverse(char *str) {
    if (!str) return NULL;
    size_t len = string_length(str);
    if (len <= 1) return str;

    size_t start = 0;
    size_t end = len - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
    return str;
}

char *string_to_upper(char *str) {
    if (!str) return NULL;
    for (char *p = str; *p != '\0'; p++) {
        *p = (char)toupper((unsigned char)*p);
    }
    return str;
}

char *string_to_lower(char *str) {
    if (!str) return NULL;
    for (char *p = str; *p != '\0'; p++) {
        *p = (char)tolower((unsigned char)*p);
    }
    return str;
}

char *string_substring(char *dest, const char *src, size_t start, size_t length) {
    if (!dest || !src) return NULL;
    size_t src_len = string_length(src);
    if (start >= src_len) {
        dest[0] = '\0';
        return dest;
    }

    size_t i = 0;
    for (i = 0; i < length && (start + i) < src_len; i++) {
        dest[i] = src[start + i];
    }
    dest[i] = '\0';
    return dest;
}

char *string_trim(char *dest, const char *src) {
    if (!dest || !src) return NULL;

    while (isspace((unsigned char)*src)) {
        src++;
    }

    if (*src == '\0') {
        dest[0] = '\0';
        return dest;
    }

    const char *end = src + string_length(src) - 1;
    while (end > src && isspace((unsigned char)*end)) {
        end--;
    }

    size_t len = (size_t)(end - src + 1);
    for (size_t i = 0; i < len; i++) {
        dest[i] = src[i];
    }
    dest[len] = '\0';
    return dest;
}

int main(void) {
    char buffer1[128];
    char buffer2[128];
    char buffer3[128];

    const char *sample_text = "  Hello, Systems Programming!  ";
    printf("Original: \"%s\"\n", sample_text);
    printf("Length: %zu\n\n", string_length(sample_text));

    string_trim(buffer1, sample_text);
    printf("Trimmed: \"%s\"\n", buffer1);

    string_copy(buffer2, buffer1);
    printf("Copied: \"%s\"\n", buffer2);

    string_concat(buffer2, " Welcome.");
    printf("Concatenated: \"%s\"\n", buffer2);

    string_substring(buffer3, buffer1, 7, 7);
    printf("Substring (offset 7, len 7): \"%s\"\n", buffer3);

    string_to_upper(buffer1);
    printf("Uppercase: \"%s\"\n", buffer1);

    string_to_lower(buffer1);
    printf("Lowercase: \"%s\"\n", buffer1);

    string_reverse(buffer1);
    printf("Reversed: \"%s\"\n", buffer1);

    int cmp1 = string_compare("Apple", "Apple");
    int cmp2 = string_compare("Apple", "Banana");
    int cmp3 = string_compare("Banana", "Apple");

    printf("\nComparisons:\n");
    printf("\"Apple\" vs \"Apple\": %d\n", cmp1);
    printf("\"Apple\" vs \"Banana\": %d\n", cmp2);
    printf("\"Banana\" vs \"Apple\": %d\n", cmp3);

    return 0;
}