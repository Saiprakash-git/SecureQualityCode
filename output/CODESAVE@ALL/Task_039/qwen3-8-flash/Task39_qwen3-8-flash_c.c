#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *shell_quote(const char *input)
{
    size_t len = strlen(input);
    char *out = malloc(4 * len + 3);
    if (!out) {
        return NULL;
    }

    char *p = out;
    *p++ = '\'';

    for (const unsigned char *s = (const unsigned char *)input; *s; ++s) {
        if (*s == '\'') {
            *p++ = '\'';
            *p++ = '\\';
            *p++ = '\'';
            *p++ = '\'';
        } else {
            *p++ = (char)*s;
        }
    }

    *p++ = '\'';
    *p = '\0';
    return out;
}

int main(void)
{
    char filename[4096];

    printf("Enter filename: ");
    fflush(stdout);

    if (!fgets(filename, sizeof(filename), stdin)) {
        fprintf(stderr, "No input.\n");
        return 1;
    }

    size_t n = strlen(filename);
    if (n > 0 && filename[n - 1] == '\n') {
        filename[n - 1] = '\0';
    }

    char *quoted = shell_quote(filename);
    if (!quoted) {
        fprintf(stderr, "Memory allocation failed.\n");
        return 1;
    }

    size_t cmd_len = strlen("cat -- ") + strlen(quoted) + 1;
    char *cmd = malloc(cmd_len);
    if (!cmd) {
        free(quoted);
        fprintf(stderr, "Memory allocation failed.\n");
        return 1;
    }

    snprintf(cmd, cmd_len, "cat -- %s", quoted);

    int status = system(cmd);

    free(cmd);
    free(quoted);

    return status == 0 ? 0 : 1;
}