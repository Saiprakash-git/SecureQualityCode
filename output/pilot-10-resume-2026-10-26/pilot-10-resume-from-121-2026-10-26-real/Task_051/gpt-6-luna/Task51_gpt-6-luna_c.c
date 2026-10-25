#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

char *encrypt_caesar(const char *input, int shift, char *output, size_t output_size)
{
    size_t length;
    int normalized_shift;

    if (input == NULL || output == NULL)
        return NULL;

    length = strlen(input);
    if (output_size < length + 1)
        return NULL;

    normalized_shift = shift % 26;
    if (normalized_shift < 0)
        normalized_shift += 26;

    for (size_t i = 0; i < length; ++i) {
        unsigned char character = (unsigned char)input[i];

        if (character >= 'A' && character <= 'Z')
            output[i] = (char)('A' + (character - 'A' + normalized_shift) % 26);
        else if (character >= 'a' && character <= 'z')
            output[i] = (char)('a' + (character - 'a' + normalized_shift) % 26);
        else
            output[i] = (char)character;
    }

    output[length] = '\0';
    return output;
}

int main(int argc, char *argv[])
{
    char input[4096];
    char output[4096];
    int shift = 3;

    if (argc > 2) {
        fprintf(stderr, "Usage: %s [shift]\n", argv[0]);
        return 1;
    }

    if (argc == 2) {
        char *end;
        long value;

        errno = 0;
        value = strtol(argv[1], &end, 10);
        if (errno != 0 || end == argv[1] || *end != '\0') {
            fprintf(stderr, "Invalid shift value.\n");
            return 1;
        }
        shift = (int)(value % 26);
    }

    if (fgets(input, sizeof(input), stdin) == NULL)
        return 1;

    input[strcspn(input, "\n")] = '\0';

    if (encrypt_caesar(input, shift, output, sizeof(output)) == NULL)
        return 1;

    puts(output);
    return 0;
}