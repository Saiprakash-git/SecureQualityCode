#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAX_INPUT_SIZE 256

int safe_read_string(char *buffer, size_t buffer_size) {
    if (buffer == NULL || buffer_size == 0) {
        return 0;
    }

    if (fgets(buffer, (int)buffer_size, stdin) == NULL) {
        return 0;
    }

    size_t len = strlen(buffer);

    if (len == buffer_size - 1 && buffer[len - 1] != '\n') {
        int c;
        while ((c = getchar()) != '\n' && c != EOF) {
        }
        return 0;
    }

    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    }

    return 1;
}

size_t process_string(const char *input, char *output, size_t output_size) {
    if (input == NULL || output == NULL || output_size == 0) {
        return 0;
    }

    size_t i = 0;
    while (input[i] != '\0' && i < output_size - 1) {
        output[i] = (char)toupper((unsigned char)input[i]);
        i++;
    }
    output[i] = '\0';

    return i;
}

int main(void) {
    char input_buffer[MAX_INPUT_SIZE];
    char output_buffer[MAX_INPUT_SIZE];

    printf("Enter a string: ");

    if (!safe_read_string(input_buffer, sizeof(input_buffer))) {
        printf("Error: Input too long or read error.\n");
        return 1;
    }

    size_t processed_len = process_string(input_buffer, output_buffer, sizeof(output_buffer));

    printf("Original: %s\n", input_buffer);
    printf("Processed: %s\n", output_buffer);
    printf("Characters processed: %zu\n", processed_len);

    return 0;
}