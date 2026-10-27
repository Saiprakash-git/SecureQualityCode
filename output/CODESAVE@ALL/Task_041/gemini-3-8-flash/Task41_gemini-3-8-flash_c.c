#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define BUFFER_SIZE 256

int read_safe_string(char *buffer, size_t max_len) {
    if (buffer == NULL || max_len == 0) {
        return -1;
    }

    if (fgets(buffer, (int)max_len, stdin) == NULL) {
        return -1;
    }

    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    } else if (len == max_len - 1) {
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF);
    }

    return 0;
}

void process_string(char *str) {
    if (str == NULL) {
        return;
    }

    for (size_t i = 0; str[i] != '\0'; i++) {
        unsigned char ch = (unsigned char)str[i];
        if (islower(ch)) {
            str[i] = (char)toupper(ch);
        } else if (isupper(ch)) {
            str[i] = (char)tolower(ch);
        }
    }

    size_t len = strlen(str);
    for (size_t i = 0, j = len > 0 ? len - 1 : 0; i < j; i++, j--) {
        char temp = str[i];
        str[i] = str[j];
        str[j] = temp;
    }
}

int main(void) {
    char input_buffer[BUFFER_SIZE];

    printf("Enter a string (up to %d characters): ", BUFFER_SIZE - 1);
    if (read_safe_string(input_buffer, sizeof(input_buffer)) != 0) {
        fprintf(stderr, "Error reading input.\n");
        return EXIT_FAILURE;
    }

    printf("Input received: %s\n", input_buffer);
    process_string(input_buffer);
    printf("Processed result (inverted case and reversed): %s\n", input_buffer);

    return EXIT_SUCCESS;
}