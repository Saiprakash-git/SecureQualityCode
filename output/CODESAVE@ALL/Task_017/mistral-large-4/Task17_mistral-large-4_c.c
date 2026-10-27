#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

char* toJadenCase(const char* str) {
    if (str == NULL || str[0] == '\0') {
        return NULL;
    }

    size_t len = strlen(str);
    char* result = (char*)malloc(len + 1);
    if (result == NULL) {
        return NULL;
    }

    int new_word = 1;
    for (size_t i = 0; i < len; i++) {
        if (new_word) {
            result[i] = toupper((unsigned char)str[i]);
            new_word = 0;
        } else {
            result[i] = tolower((unsigned char)str[i]);
        }

        if (str[i] == ' ') {
            new_word = 1;
        }
    }
    result[len] = '\0';
    return result;
}

int main() {
    const char* test1 = "How can mirrors be real if our eyes aren't real";
    char* result1 = toJadenCase(test1);
    if (result1 != NULL) {
        printf("Input:  \"%s\"\n", test1);
        printf("Output: \"%s\"\n", result1);
        free(result1);
    }

    char* result2 = toJadenCase("");
    printf("Empty string: %s\n", result2 == NULL ? "NULL" : result2);

    char* result3 = toJadenCase(NULL);
    printf("NULL input: %s\n", result3 == NULL ? "NULL" : result3);

    return 0;
}