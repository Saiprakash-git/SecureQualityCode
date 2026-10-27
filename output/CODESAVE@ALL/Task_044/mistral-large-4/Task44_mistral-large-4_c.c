#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#define MAX_USERNAME_LEN 32
#define MAX_MESSAGE_LEN 256
#define MAX_INPUT_LEN 512

bool read_line(char *buffer, size_t size) {
    if (buffer == NULL || size == 0) {
        return false;
    }
    if (fgets(buffer, (int)size, stdin) == NULL) {
        return false;
    }
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    } else if (len == size - 1) {
        int c;
        while ((c = getchar()) != '\n' && c != EOF) {
            if (c == EOF) {
                return false;
            }
        }
    }
    return true;
}

bool validate_username(const char *username) {
    if (username == NULL) {
        return false;
    }
    size_t len = strlen(username);
    if (len == 0 || len > MAX_USERNAME_LEN) {
        return false;
    }
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)username[i];
        if (!isalnum(c) && c != '_') {
            return false;
        }
    }
    return true;
}

bool validate_integer(const char *str, int min, int max, int *result) {
    if (str == NULL || result == NULL || strlen(str) == 0) {
        return false;
    }
    char *endptr;
    errno = 0;
    long val = strtol(str, &endptr, 10);
    if (errno != 0 || *endptr != '\0' || val < min || val > max) {
        return false;
    }
    *result = (int)val;
    return true;
}

void reverse_string(char *str) {
    if (str == NULL) {
        return;
    }
    size_t len = strlen(str);
    for (size_t i = 0; i < len / 2; i++) {
        char temp = str[i];
        str[i] = str[len - 1 - i];
        str[len - 1 - i] = temp;
    }
}

void process_data(const char *username, int number, const char *message) {
    char reversed[MAX_MESSAGE_LEN + 1];
    strncpy(reversed, message, MAX_MESSAGE_LEN);
    reversed[MAX_MESSAGE_LEN] = '\0';
    reverse_string(reversed);

    printf("Username: %s\n", username);
    printf("Number: %d (squared: %d)\n", number, number * number);
    printf("Message: %s\n", message);
    printf("Reversed: %s\n", reversed);
    printf("Message length: %zu\n", strlen(message));
}

int main(void) {
    char username[MAX_USERNAME_LEN + 1];
    char number_str[32];
    char message[MAX_MESSAGE_LEN + 1];
    int number;

    printf("Enter username (alphanumeric and underscore, max %d chars): ", MAX_USERNAME_LEN);
    if (!read_line(username, sizeof(username))) {
        fprintf(stderr, "Error reading username\n");
        return 1;
    }
    if (!validate_username(username)) {
        fprintf(stderr, "Invalid username\n");
        return 1;
    }

    printf("Enter a number (1-1000): ");
    if (!read_line(number_str, sizeof(number_str))) {
        fprintf(stderr, "Error reading number\n");
        return 1;
    }
    if (!validate_integer(number_str, 1, 1000, &number)) {
        fprintf(stderr, "Invalid number\n");
        return 1;
    }

    printf("Enter a message (max %d chars): ", MAX_MESSAGE_LEN);
    if (!read_line(message, sizeof(message))) {
        fprintf(stderr, "Error reading message\n");
        return 1;
    }

    process_data(username, number, message);
    return 0;
}