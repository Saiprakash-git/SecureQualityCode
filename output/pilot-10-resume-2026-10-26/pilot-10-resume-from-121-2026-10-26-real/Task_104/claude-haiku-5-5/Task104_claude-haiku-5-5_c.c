/* Reads user input into a fixed-size buffer safely. */
#include <stdio.h>
#include <string.h>

#define BUFFER_SIZE 64

static int read_line_into_buffer(char *buffer, size_t size, FILE *stream)
{
    if (buffer == NULL || size == 0 || stream == NULL) {
        return -1;
    }

    if (fgets(buffer, (int)size, stream) == NULL) {
        buffer[0] = '\0';
        return -1;
    }

    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
        return (int)(len - 1);
    }

    /* Input was longer than the buffer: discard the rest of the line. */
    int ch;
    while ((ch = fgetc(stream)) != '\n' && ch != EOF) {
        /* discard */
    }
    return (int)len;
}

static void process_buffer(const char *buffer, size_t length)
{
    printf("You entered (%zu chars): \"%s\"\n", length, buffer);
}

int main(void)
{
    char buffer[BUFFER_SIZE];

    printf("Enter text (max %d chars): ", BUFFER_SIZE - 1);

    int len = read_line_into_buffer(buffer, sizeof(buffer), stdin);
    if (len < 0) {
        fprintf(stderr, "Error: failed to read input.\n");
        return 1;
    }

    process_buffer(buffer, (size_t)len);
    return 0;
}