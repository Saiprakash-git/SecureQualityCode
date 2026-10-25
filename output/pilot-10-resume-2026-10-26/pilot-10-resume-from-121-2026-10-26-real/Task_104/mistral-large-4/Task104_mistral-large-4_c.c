#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define BUFFER_SIZE 256

bool read_into_buffer(char *buffer, size_t buffer_size) {
    if (buffer == NULL || buffer_size == 0) {
        return false;
    }

    if (fgets(buffer, (int)buffer_size, stdin) == NULL) {
        return false;
    }

    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    } else if (len == buffer_size - 1) {
        int c;
        while ((c = getchar()) != '\n' && c != EOF) {
        }
    }

    return true;
}

size_t get_buffer_length(const char *buffer) {
    if (buffer == NULL) {
        return 0;
    }
    return strlen(buffer);
}

void print_buffer(const char *buffer) {
    if (buffer != NULL) {
        printf("%s\n", buffer);
    }
}

int main(void) {
    char fixed_buffer[BUFFER_SIZE];

    printf("Enter input: ");

    if (read_into_buffer(fixed_buffer, BUFFER_SIZE)) {
        printf("You entered: ");
        print_buffer(fixed_buffer);
        printf("Length: %zu\n", get_buffer_length(fixed_buffer));
    } else {
        printf("Input error or EOF\n");
        return 1;
    }

    return 0;
}