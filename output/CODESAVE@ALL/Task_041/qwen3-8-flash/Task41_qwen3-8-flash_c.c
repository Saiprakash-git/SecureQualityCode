#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAX_INPUT 256

int main(void) {
    char input[MAX_INPUT];

    printf("Enter a string: ");

    if (fgets(input, sizeof(input), stdin) == NULL) {
        return 1;
    }

    size_t length = strlen(input);

    if (length > 0 && input[length - 1] == '\n') {
        input[length - 1] = '\0';
        length--;
    }

    for (size_t i = 0; i < length; i++) {
        input[i] = (char)toupper((unsigned char)input[i]);
    }

    printf("Processed string: %s\n", input);
    printf("String length: %zu\n", length);

    return 0;
}