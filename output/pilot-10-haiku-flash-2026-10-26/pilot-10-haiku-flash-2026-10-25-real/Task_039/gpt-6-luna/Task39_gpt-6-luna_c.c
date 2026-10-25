#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/wait.h>

static char *shell_quote(const char *text)
{
    size_t length = strlen(text);
    size_t apostrophes = 0;

    for (const char *p = text; *p; ++p) {
        if (*p == '\'')
            ++apostrophes;
    }

    if (length > SIZE_MAX - 3 ||
        apostrophes > (SIZE_MAX - length - 3) / 3)
        return NULL;

    size_t capacity = length + 3 * apostrophes + 3;
    char *quoted = malloc(capacity);
    if (!quoted)
        return NULL;

    char *out = quoted;
    *out++ = '\'';

    for (const char *p = text; *p; ++p) {
        if (*p == '\'') {
            memcpy(out, "'\\''", 4);
            out += 4;
        } else {
            *out++ = *p;
        }
    }

    *out++ = '\'';
    *out = '\0';
    return quoted;
}

int main(void)
{
    char *filename = NULL;
    size_t capacity = 0;

    printf("Enter filename: ");
    fflush(stdout);

    ssize_t length = getline(&filename, &capacity, stdin);
    if (length < 0) {
        fprintf(stderr, "Could not read filename.\n");
        free(filename);
        return 1;
    }

    if (memchr(filename, '\0', (size_t)length) != NULL) {
        fprintf(stderr, "Filename contains an invalid null byte.\n");
        free(filename);
        return 1;
    }

    if (length > 0 && filename[length - 1] == '\n')
        filename[--length] = '\0';

    if (length == 0) {
        fprintf(stderr, "Filename cannot be empty.\n");
        free(filename);
        return 1;
    }

    char *quoted = shell_quote(filename);
    free(filename);
    if (!quoted) {
        fprintf(stderr, "Could not prepare filename.\n");
        return 1;
    }

    size_t quoted_length = strlen(quoted);
    if (quoted_length > SIZE_MAX - 8) {
        fprintf(stderr, "Filename is too long.\n");
        free(quoted);
        return 1;
    }

    char *command = malloc(quoted_length + 8);
    if (!command) {
        fprintf(stderr, "Could not prepare command.\n");
        free(quoted);
        return 1;
    }

    snprintf(command, quoted_length + 8, "cat -- %s", quoted);
    free(quoted);

    int result = system(command);
    free(command);

    if (result == -1) {
        perror("system");
        return 1;
    }
    if (WIFEXITED(result))
        return WEXITSTATUS(result);
    if (WIFSIGNALED(result))
        return 128 + WTERMSIG(result);

    return 1;
}