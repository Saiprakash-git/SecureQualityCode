#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

enum LineStatus {
    LINE_ERROR = -1,
    LINE_EOF = 0,
    LINE_OK = 1
};

static int read_line(FILE *input, char *buffer, size_t capacity)
{
    size_t length = 0;
    int too_long = 0;
    int invalid = 0;
    int received_any = 0;

    if (input == NULL || buffer == NULL || capacity < 2) {
        return LINE_ERROR;
    }

    for (;;) {
        int ch = fgetc(input);

        if (ch == EOF) {
            if (ferror(input)) {
                return LINE_ERROR;
            }
            if (!received_any) {
                return LINE_EOF;
            }
            break;
        }

        received_any = 1;

        if (ch == '\n') {
            break;
        }

        if (ch == '\0') {
            invalid = 1;
            continue;
        }

        if (length + 1 < capacity) {
            buffer[length++] = (char)ch;
        } else {
            too_long = 1;
        }
    }

    buffer[length] = '\0';

    if (too_long || invalid) {
        return LINE_ERROR;
    }

    return LINE_OK;
}

static int parse_number(const char *text, double *result)
{
    char *end;
    double value;

    if (text == NULL || result == NULL) {
        return 0;
    }

    errno = 0;
    value = strtod(text, &end);

    if (end == text || errno == ERANGE || !isfinite(value)) {
        return 0;
    }

    while (*end != '\0' && isspace((unsigned char)*end)) {
        end++;
    }

    if (*end != '\0' || fabs(value) > 1.0e100) {
        return 0;
    }

    *result = value;
    return 1;
}

static int parse_operator(const char *text, char *operation)
{
    const char *position;
    char parsed;

    if (text == NULL || operation == NULL) {
        return 0;
    }

    position = text;
    while (*position != '\0' && isspace((unsigned char)*position)) {
        position++;
    }

    parsed = *position;
    if (parsed != '+' && parsed != '-' && parsed != '*' && parsed != '/') {
        return 0;
    }

    position++;
    while (*position != '\0' && isspace((unsigned char)*position)) {
        position++;
    }

    if (*position != '\0') {
        return 0;
    }

    *operation = parsed;
    return 1;
}

static int calculate(double left, char operation, double right, double *result)
{
    double value;

    if (result == NULL) {
        return 0;
    }

    switch (operation) {
        case '+':
            value = left + right;
            break;
        case '-':
            value = left - right;
            break;
        case '*':
            value = left * right;
            break;
        case '/':
            if (right == 0.0) {
                return 0;
            }
            value = left / right;
            break;
        default:
            return 0;
    }

    if (!isfinite(value)) {
        return 0;
    }

    *result = value;
    return 1;
}

static int prompt_number(const char *prompt, double *value)
{
    char line[128];
    int status;

    fputs(prompt, stdout);
    fflush(stdout);

    status = read_line(stdin, line, sizeof line);
    if (status == LINE_EOF) {
        fputs("\nInput ended.\n", stderr);
        return 0;
    }
    if (status != LINE_OK || !parse_number(line, value)) {
        fputs("Invalid input. Enter a finite decimal number within the supported range.\n",
              stderr);
        return 0;
    }

    return 1;
}

int main(void)
{
    char line[128];
    char operation;
    double left;
    double right;
    double result;
    int status;

    if (!prompt_number("Enter the first number: ", &left)) {
        return EXIT_FAILURE;
    }

    fputs("Enter an operation (+, -, *, /): ", stdout);
    fflush(stdout);

    status = read_line(stdin, line, sizeof line);
    if (status == LINE_EOF) {
        fputs("\nInput ended.\n", stderr);
        return EXIT_FAILURE;
    }
    if (status != LINE_OK || !parse_operator(line, &operation)) {
        fputs("Invalid operation. Choose exactly one of +, -, *, or /.\n", stderr);
        return EXIT_FAILURE;
    }

    if (!prompt_number("Enter the second number: ", &right)) {
        return EXIT_FAILURE;
    }

    if (!calculate(left, operation, right, &result)) {
        fputs("Cannot calculate the result (division by zero or numeric overflow).\n",
              stderr);
        return EXIT_FAILURE;
    }

    printf("Result: %.17g\n", result);
    return EXIT_SUCCESS;
}