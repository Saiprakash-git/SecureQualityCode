#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

char *jaden_case(const char *s) {
    if (s == NULL || s[0] == '\0') {
        return NULL;
    }

    size_t n = strlen(s);
    char *result = malloc(n + 1);
    if (result == NULL) {
        return NULL;
    }

    int start_of_word = 1;
    for (size_t i = 0; i < n; i++) {
        unsigned char c = (unsigned char)s[i];
        if (isspace(c)) {
            result[i] = (char)c;
            start_of_word = 1;
        } else if (start_of_word) {
            result[i] = (char)toupper(c);
            start_of_word = 0;
        } else {
            result[i] = (char)tolower(c);
        }
    }
    result[n] = '\0';
    return result;
}

int main(void) {
    const char *input = "How can mirrors be real if our eyes aren't real";
    char *output = jaden_case(input);
    if (output != NULL) {
        printf("%s\n", output);
        free(output);
    } else {
        printf("NULL\n");
    }
    return 0;
}