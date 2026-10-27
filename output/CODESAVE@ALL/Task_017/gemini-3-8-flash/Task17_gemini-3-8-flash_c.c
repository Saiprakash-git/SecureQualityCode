#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

char *to_jaden_case(const char *string) {
    if (string == NULL || *string == '\0') {
        return NULL;
    }

    size_t len = strlen(string);
    char *result = (char *)malloc(len + 1);
    if (result == NULL) {
        return NULL;
    }

    int capitalize_next = 1;
    for (size_t i = 0; i < len; i++) {
        if (capitalize_next && islower((unsigned char)string[i])) {
            result[i] = (char)toupper((unsigned char)string[i]);
        } else {
            result[i] = string[i];
        }
        capitalize_next = (string[i] == ' ');
    }
    result[len] = '\0';

    return result;
}

int main(void) {
    const char *quote = "How can mirrors be real if our eyes aren't real";
    char *jaden_cased = to_jaden_case(quote);

    if (jaden_cased != NULL) {
        printf("Not Jaden-Cased: \"%s\"\n", quote);
        printf("Jaden-Cased:     \"%s\"\n", jaden_cased);
        free(jaden_cased);
    }

    return 0;
}