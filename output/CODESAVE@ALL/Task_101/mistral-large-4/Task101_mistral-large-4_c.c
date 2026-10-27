#include <stdio.h>
#include <sys/stat.h>

int create_executable_script(const char *filename) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        return -1;
    }
    fprintf(file, "#!/bin/bash\necho \"Hello, World!\"\n");
    fclose(file);
    if (chmod(filename, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) != 0) {
        return -1;
    }
    return 0;
}

int main(void) {
    if (create_executable_script("script.sh") != 0) {
        return 1;
    }
    return 0;
}