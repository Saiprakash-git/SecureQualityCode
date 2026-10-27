#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* concatenate_strings(int count, char** strings) {
    if (count <= 0 || strings == NULL) {
        char* empty = malloc(1);
        if (empty != NULL) empty[0] = '\0';
        return empty;
    }

    size_t total_length = 0;
    for (int i = 0; i < count; i++) {
        if (strings[i] != NULL) {
            total_length += strlen(strings[i]);
        }
    }

    char* result = malloc(total_length + 1);
    if (result == NULL) return NULL;

    result[0] = '\0';
    for (int i = 0; i < count; i++) {
        if (strings[i] != NULL) {
            strcat(result, strings[i]);
        }
    }

    return result;
}

int main(void) {
    char** strings = NULL;
    int count = 0;
    int capacity = 0;
    char buffer[1024];

    printf("Enter strings (press Enter on empty line to finish):\n");

    while (1) {
        printf("String %d: ", count + 1);
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            break;
        }

        buffer[strcspn(buffer, "\n")] = '\0';

        if (buffer[0] == '\0') {
            break;
        }

        if (count >= capacity) {
            int new_capacity = (capacity == 0) ? 4 : capacity * 2;
            char** temp = realloc(strings, new_capacity * sizeof(char*));
            if (temp == NULL) {
                for (int i = 0; i < count; i++) free(strings[i]);
                free(strings);
                return 1;
            }
            strings = temp;
            capacity = new_capacity;
        }

        strings[count] = malloc(strlen(buffer) + 1);
        if (strings[count] == NULL) {
            for (int i = 0; i < count; i++) free(strings[i]);
            free(strings);
            return 1;
        }
        strcpy(strings[count], buffer);
        count++;
    }

    if (count == 0) {
        printf("No strings provided.\n");
        free(strings);
        return 0;
    }

    char* result = concatenate_strings(count, strings);
    if (result != NULL) {
        printf("Concatenated string: %s\n", result);
        free(result);
    } else {
        printf("Memory allocation failed.\n");
    }

    for (int i = 0; i < count; i++) {
        free(strings[i]);
    }
    free(strings);

    return 0;
}