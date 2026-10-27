#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int read_filename(char *buf, size_t size) {
    if (fgets(buf, (int)size, stdin) == NULL) {
        return -1;
    }
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
        len--;
    }
    if (len == 0) {
        return -1;
    }
    return 0;
}

static int shell_quote(const char *in, char *out, size_t size) {
    size_t pos = 0;
    if (size < 3) {
        return -1;
    }
    out[pos++] = '\'';
    for (const char *p = in; *p != '\0'; p++) {
        if (*p == '\'') {
            if (pos + 4 >= size) {
                return -1;
            }
            out[pos++] = '\'';
            out[pos++] = '\\';
            out[pos++] = '\'';
            out[pos++] = '\'';
        } else {
            if (pos + 2 >= size) {
                return -1;
            }
            out[pos++] = *p;
        }
    }
    if (pos + 2 > size) {
        return -1;
    }
    out[pos++] = '\'';
    out[pos] = '\0';
    return 0;
}

static int build_command(const char *filename, char *cmd, size_t size) {
    char quoted[1024];
    if (shell_quote(filename, quoted, sizeof(quoted)) != 0) {
        return -1;
    }
    int n = snprintf(cmd, size, "cat %s", quoted);
    if (n < 0 || (size_t)n >= size) {
        return -1;
    }
    return 0;
}

static int display_file(const char *filename) {
    char cmd[2048];
    if (build_command(filename, cmd, sizeof(cmd)) != 0) {
        fprintf(stderr, "Error: filename is too long or invalid.\n");
        return -1;
    }
    int status = system(cmd);
    if (status == -1) {
        fprintf(stderr, "Error: failed to execute system command.\n");
        return -1;
    }
    if (status != 0) {
        fprintf(stderr, "Error: could not display file '%s'.\n", filename);
        return -1;
    }
    return 0;
}

int main(void) {
    char filename[512];

    printf("Enter filename: ");
    fflush(stdout);

    if (read_filename(filename, sizeof(filename)) != 0) {
        fprintf(stderr, "Error: no filename provided.\n");
        return EXIT_FAILURE;
    }

    if (display_file(filename) != 0) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}