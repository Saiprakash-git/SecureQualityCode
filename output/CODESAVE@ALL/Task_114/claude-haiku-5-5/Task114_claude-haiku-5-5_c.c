#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

size_t str_length(const char *s) {
    size_t n = 0;
    while (s[n] != '\0') n++;
    return n;
}

char *str_copy(char *dest, const char *src) {
    char *d = dest;
    while ((*d++ = *src++) != '\0');
    return dest;
}

char *str_ncopy(char *dest, const char *src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++) dest[i] = src[i];
    for (; i < n; i++) dest[i] = '\0';
    return dest;
}

char *str_concat(char *dest, const char *src) {
    char *d = dest;
    while (*d) d++;
    while ((*d++ = *src++) != '\0');
    return dest;
}

int str_compare(const char *a, const char *b) {
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

char *str_duplicate(const char *s) {
    size_t len = str_length(s);
    char *p = malloc(len + 1);
    if (!p) return NULL;
    str_copy(p, s);
    return p;
}

char *str_reverse(char *s) {
    size_t len = str_length(s);
    for (size_t i = 0; i < len / 2; i++) {
        char tmp = s[i];
        s[i] = s[len - 1 - i];
        s[len - 1 - i] = tmp;
    }
    return s;
}

char *str_to_upper(char *s) {
    for (char *p = s; *p; p++) *p = (char)toupper((unsigned char)*p);
    return s;
}

char *str_to_lower(char *s) {
    for (char *p = s; *p; p++) *p = (char)tolower((unsigned char)*p);
    return s;
}

char *str_trim(char *s) {
    char *start = s;
    while (isspace((unsigned char)*start)) start++;
    char *end = start + str_length(start);
    while (end > start && isspace((unsigned char)*(end - 1))) end--;
    size_t len = (size_t)(end - start);
    memmove(s, start, len);
    s[len] = '\0';
    return s;
}

int str_count_char(const char *s, char c) {
    int count = 0;
    for (; *s; s++) if (*s == c) count++;
    return count;
}

char *str_replace_char(char *s, char from, char to) {
    for (char *p = s; *p; p++) if (*p == from) *p = to;
    return s;
}

int str_is_palindrome(const char *s) {
    size_t len = str_length(s);
    if (len == 0) return 1;
    size_t i = 0, j = len - 1;
    while (i < j) {
        if (s[i] != s[j]) return 0;
        i++;
        j--;
    }
    return 1;
}

char *str_substring(const char *s, size_t start, size_t len) {
    size_t slen = str_length(s);
    if (start > slen) start = slen;
    if (len > slen - start) len = slen - start;
    char *out = malloc(len + 1);
    if (!out) return NULL;
    memcpy(out, s + start, len);
    out[len] = '\0';
    return out;
}

int str_find(const char *haystack, const char *needle) {
    size_t hlen = str_length(haystack);
    size_t nlen = str_length(needle);
    if (nlen == 0) return 0;
    if (nlen > hlen) return -1;
    for (size_t i = 0; i <= hlen - nlen; i++) {
        if (memcmp(haystack + i, needle, nlen) == 0) return (int)i;
    }
    return -1;
}

char *str_remove_char(char *s, char c) {
    char *r = s, *w = s;
    while (*r) {
        if (*r != c) *w++ = *r;
        r++;
    }
    *w = '\0';
    return s;
}

int main(void) {
    char buffer[256];
    char text[256];

    str_copy(buffer, "  Hello, World  ");
    printf("Original: [%s]\n", buffer);

    str_trim(buffer);
    printf("Trimmed: [%s]\n", buffer);

    str_copy(text, buffer);
    str_concat(text, "!!!");
    printf("Concatenated: %s\n", text);

    str_to_upper(text);
    printf("Uppercase: %s\n", text);

    str_to_lower(text);
    printf("Lowercase: %s\n", text);

    str_reverse(text);
    printf("Reversed: %s\n", text);

    printf("Length: %zu\n", str_length(buffer));
    printf("Count of 'l': %d\n", str_count_char(buffer, 'l'));

    str_replace_char(buffer, 'l', 'L');
    printf("Replaced 'l' with 'L': %s\n", buffer);

    printf("Find \"World\": %d\n", str_find(buffer, "World"));

    char *sub = str_substring("programming", 3, 4);
    if (sub) {
        printf("Substring: %s\n", sub);
        free(sub);
    }

    char *dup = str_duplicate("duplicate");
    if (dup) {
        printf("Duplicate: %s\n", dup);
        free(dup);
    }

    printf("Compare 'abc' vs 'abd': %d\n", str_compare("abc", "abd"));
    printf("Palindrome 'racecar': %s\n", str_is_palindrome("racecar") ? "yes" : "no");
    printf("Palindrome 'hello': %s\n", str_is_palindrome("hello") ? "yes" : "no");

    str_remove_char(buffer, 'o');
    printf("Removed 'o': %s\n", buffer);

    str_ncopy(text, "truncate", 4);
    text[4] = '\0';
    printf("ncopy: %s\n", text);

    return 0;
}