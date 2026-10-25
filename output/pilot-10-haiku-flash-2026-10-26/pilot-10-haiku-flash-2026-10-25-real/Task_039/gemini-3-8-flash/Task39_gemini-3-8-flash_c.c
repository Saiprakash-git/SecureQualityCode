#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
#define VIEW_CMD "type"
#else
#define VIEW_CMD "cat"
#endif

int display_file(const char *filename) {
    if (filename == NULL || filename[0] == '\0') {
        return -1;
    }

    char command[1024];
    int written = snprintf(command, sizeof(command), "%s \"%s\"", VIEW_CMD, filename);
    if (written < 0 || (size_t)written >= sizeof(command)) {
        return -1;
    }

    return system(command);
}

int main(void) {
    char filename[512];

    printf("Enter filename: ");
    if (fgets(filename, sizeof(filename), stdin) == NULL) {
        return 1;
    }

    filename[strcspn(filename, "\r\n")] = '\0';

    if (filename[0] == '\0') {
        fprintf(stderr, "Error: Filename cannot be empty.\n");
        return 1;
    }

    int result = display_file(filename);
    if (result != 0) {
        fprintf(stderr, "Failed to display file.\n");
        return 1;
    }

    return 0;
}