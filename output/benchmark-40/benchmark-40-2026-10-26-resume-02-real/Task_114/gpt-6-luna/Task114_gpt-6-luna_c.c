#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define TEXT_CAPACITY 1024
#define RESULT_CAPACITY (TEXT_CAPACITY * 2)

static int read_line(const char *prompt, char *buffer, size_t capacity)
{
    size_t length;
    int ch;

    if (capacity == 0) {
        return 0;
    }

    printf("%s", prompt);
    if (fgets(buffer, (int)capacity, stdin) == NULL) {
        return 0;
    }

    length = strlen(buffer);
    if (length > 0 && buffer[length - 1] == '\n') {
        buffer[--length] = '\0';
    } else if (!feof(stdin)) {
        while ((ch = getchar()) != '\n' && ch != EOF) {
        }
        fprintf(stderr, "Input is too long.\n");
        return 0;
    }

    if (length > 0 && buffer[length - 1] == '\r') {
        buffer[length - 1] = '\0';
    }

    return 1;
}

static int string_copy(char *destination, size_t capacity, const char *source)
{
    size_t length = strlen(source);

    if (capacity == 0 || length >= capacity) {
        return 0;
    }

    memcpy(destination, source, length + 1);
    return 1;
}

static int string_concatenate(char *destination, size_t capacity,
                              const char *first, const char *second)
{
    size_t first_length = strlen(first);
    size_t second_length = strlen(second);

    if (capacity == 0 || first_length >= capacity ||
        second_length >= capacity - first_length) {
        return 0;
    }

    memcpy(destination, first, first_length);
    memcpy(destination + first_length, second, second_length + 1);
    return 1;
}

static size_t string_length(const char *text)
{
    return strlen(text);
}

static int string_compare(const char *first, const char *second)
{
    return strcmp(first, second);
}

static int string_reverse(char *destination, size_t capacity,
                          const char *source)
{
    size_t length = strlen(source);
    size_t i;

    if (capacity == 0 || length >= capacity) {
        return 0;
    }

    for (i = 0; i < length; ++i) {
        destination[i] = source[length - i - 1];
    }
    destination[length] = '\0';
    return 1;
}

static int string_convert_case(char *destination, size_t capacity,
                               const char *source, int to_upper)
{
    size_t length = strlen(source);
    size_t i;

    if (capacity == 0 || length >= capacity) {
        return 0;
    }

    for (i = 0; i < length; ++i) {
        unsigned char character = (unsigned char)source[i];
        destination[i] = (char)(to_upper ? toupper(character) : tolower(character));
    }
    destination[length] = '\0';
    return 1;
}

int main(void)
{
    char first[TEXT_CAPACITY];
    char second[TEXT_CAPACITY];
    char copied[TEXT_CAPACITY];
    char concatenated[RESULT_CAPACITY];
    char reversed[TEXT_CAPACITY];
    char uppercase[TEXT_CAPACITY];
    char lowercase[TEXT_CAPACITY];
    int comparison;

    if (!read_line("Enter the first string: ", first, sizeof(first)) ||
        !read_line("Enter the second string: ", second, sizeof(second))) {
        return 1;
    }

    printf("\nLength of first string: %zu\n", string_length(first));
    printf("Length of second string: %zu\n", string_length(second));

    if (string_copy(copied, sizeof(copied), first)) {
        printf("Copy of first string: %s\n", copied);
    }

    if (string_concatenate(concatenated, sizeof(concatenated), first, second)) {
        printf("Concatenation: %s\n", concatenated);
    }

    if (string_reverse(reversed, sizeof(reversed), first)) {
        printf("Reversed first string: %s\n", reversed);
    }

    if (string_convert_case(uppercase, sizeof(uppercase), first, 1)) {
        printf("First string in uppercase: %s\n", uppercase);
    }

    if (string_convert_case(lowercase, sizeof(lowercase), first, 0)) {
        printf("First string in lowercase: %s\n", lowercase);
    }

    comparison = string_compare(first, second);
    if (comparison < 0) {
        printf("Comparison: the first string comes before the second.\n");
    } else if (comparison > 0) {
        printf("Comparison: the first string comes after the second.\n");
    } else {
        printf("Comparison: the strings are equal.\n");
    }

    return 0;
}