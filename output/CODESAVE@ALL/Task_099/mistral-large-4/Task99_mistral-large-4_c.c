#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>

int process_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

    regex_t regex;
    int reti = regcomp(&regex, "^[0-9]+.*\\.$", REG_EXTENDED | REG_NOSUB);
    if (reti != 0) {
        fprintf(stderr, "Could not compile regex\n");
        fclose(file);
        return 1;
    }

    char