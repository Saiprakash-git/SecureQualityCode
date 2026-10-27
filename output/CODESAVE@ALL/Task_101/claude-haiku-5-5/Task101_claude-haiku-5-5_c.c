#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

static int create_script(const char *path, const char *content) {
    FILE *fp = fopen(path, "w");
    if (fp == NULL) {
        return -1;
    }
    if (fputs(content, fp) == EOF) {
        fclose(fp);
        return -1;
    }
    if (fclose(fp) != 0) {
        return -1;
    }
    return 0;
}

static int make_executable(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return -1;
    }
    mode_t mode = st.st_mode | S_IXUSR | S_IXGRP | S_IXOTH;
    return chmod(path, mode);
}

int main(void) {
    const char *filename = "script.sh";
    const char *content =
        "#!/bin/sh\n"
        "echo \"Hello from script.sh\"\n";

    if (create_script(filename, content) != 0) {
        perror("Failed to create script.sh");
        return EXIT_FAILURE;
    }

    if (make_executable(filename) != 0) {
        perror("Failed to make script.sh executable");
        return EXIT_FAILURE;
    }

    printf("Created and made executable: %s\n", filename);
    return EXIT_SUCCESS;
}