#include <stdio.h>
#include <string.h>

#define INPUT_BUFFER_SIZE 64

int read_user_input(char *buffer, size_t buffer_size)
{
    if (buffer == NULL || buffer_size == 0)
        return -1;

    if (fgets(buffer, (int)buffer_size, stdin) == NULL)
        return -2;

    size_t length = strlen(buffer);
    if (length > 0 && buffer[length - 1] == '\n')
        buffer[length - 1] = '\0';

    return 0;
}

int process_user_input(const char *input, char *output, size_t output_size)
{
    if (input == NULL || output == NULL || output_size == 0)
        return -1;

    size_t input_length = strlen(input);

    if (input_length == 0)
    {
        snprintf(output, output_size, "No input received.");
        return 0;
    }

    if (input_length >= INPUT_BUFFER_SIZE - 1)
    {
        snprintf(output, output_size, "Input was truncated at %d characters.", INPUT_BUFFER_SIZE - 1);
        return 0;
    }

    snprintf(output, output_size, "Received input: %s", input);
    return 0;
}

int main(void)
{
    char input[INPUT_BUFFER_SIZE];
    char output[INPUT_BUFFER_SIZE * 2];

    if (read_user_input(input, sizeof(input)) != 0)
    {
        fprintf(stderr, "Failed to read input.\n");
        return 1;
    }

    if (process_user_input(input, output, sizeof(output)) != 0)
    {
        fprintf(stderr, "Failed to process input.\n");
        return 1;
    }

    printf("%s\n", output);
    return 0;
}