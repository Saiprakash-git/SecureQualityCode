#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_INPUT 1024

static size_t read_line_safe(char *buf, size_t size) {
    if (fgets(buf, (int)size, stdin) == NULL) {
        buf[0] = '\0';
        return 0;
    }
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
        len--;
    } else {
        int c;
        while ((c = getchar()) != '\n' && c != EOF) {
        }
    }
    return len;
}

static void to_upper_copy(char *dst, const char *src, size_t dst_size) {
    size_t i;
    for (i = 0; i + 1 < dst_size && src[i] != '\0'; i++) {
        dst[i] = (char)toupper((unsigned char)src[i]);
    }
    dst[i] = '\0';
}

static size_t count_words(const char *s) {
    size_t count = 0;
    int in_word = 0;
    for (; *s; s++) {
        if (isspace((unsigned char)*s)) {
            in_word = 0;
        } else if (!in_word) {
            in_word = 1;
            count++;
        }
    }
    return count;
}

static void reverse_string(char *s) {
    size_t len = strlen(s);
    for (size_t i = 0, j = len; i < j / 2 + (j % 2 ? 0 : 0) && i < len / 2; i++) {
        char tmp = s[i];
        s[i] = s[len - 1 - i];
        s[len - 1 - i] = tmp;
    }
}

static size_t count_vowels(const char *s) {
    size_t count = 0;
    for (; *s; s++) {
        switch (tolower((unsigned char)*s)) {
            case 'a': case 'e': case 'i': case 'o': case 'u':
                count++;
                break;
            default:
                break;
        }
    }
    return count;
}

int main(void) {
    char input[MAX_INPUT];
    char upper[MAX_INPUT];
    char reversed[MAX_INPUT];

    printf("Enter a string (max %d characters): ", MAX_INPUT - 1);
    fflush(stdout);

    size_t len = read_line_safe(input, sizeof(input));
    if (len == 0) {
        printf("No input provided.\n");
        return EXIT_SUCCESS;
    }

    to_upper_copy(upper, input, sizeof(upper));
    strncpy(reversed, input, sizeof(reversed) - 1);
    reversed[sizeof(reversed) - 1] = '\0';
    reverse_string(reversed);

    printf("Original:   %s\n", input);
    printf("Uppercase:  %s\n", upper);
    printf("Reversed:   %s\n", reversed);
    printf("Length:     %zu\n", strlen(input));
    printf("Words:      %zu\n", count_words(input));
    printf("Vowels:     %zu\n", count_vowels(input));

    return EXIT_SUCCESS;
}