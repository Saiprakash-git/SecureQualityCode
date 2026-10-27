#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void display_file_contents(const char *filename) {
    char command[512];
#ifdef _WIN32
    snprintf(command, sizeof(command), "type \"%s\"", filename);
#else
    snprintf(command, sizeof(command), "cat \"%s\"", filename);
#endif
    system(command);
}

int main(void) {
    char filename[256];

    printf("Enter filename: ");
    if (fgets(filename, sizeof(filename), stdin) == NULL) {
        return 1;
    }

    filename[strcspn(filename, "\n")] = '\0';

    display_file_contents(filename);

    return 0;
}