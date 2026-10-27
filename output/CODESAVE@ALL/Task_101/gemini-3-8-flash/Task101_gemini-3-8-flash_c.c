#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

int create_executable_script(const char *filepath, const char *content) {
    if (filepath == NULL || content == NULL) {
        return -1;
    }

    FILE *fp = fopen(filepath, "w");
    if (fp == NULL) {
        perror("Error opening file");
        return -1;
    }

    if (fputs(content, fp) == EOF) {
        perror("Error writing to file");
        fclose(fp);
        return -1;
    }

    if (fclose(fp) != 0) {
        perror("Error closing file");
        return -1;
    }

    mode_t mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
    if (chmod(filepath, mode) != 0) {
        perror("Error setting file permissions");
        return -1;
    }

    return 0;
}

int main(void) {
    const char *filename = "script.sh";
    const char *script_content = "#!/bin/sh\necho \"Script executed successfully.\"\n";

    if (create_executable_script(filename, script_content) != 0) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}