#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_CMD_LEN 4096
#define MAX_OUTPUT_LEN 65536

int validate_command(const char *cmd) {
    if (cmd == NULL) return 0;
    size_t len = strlen(cmd);
    if (len == 0 || len >= MAX_CMD_LEN) return 0;
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)cmd[i];
        if (c == 0) return 0;
        if (c < 32 && c != '\t') return 0;
    }
    return 1;
}

char *execute_command(const char *cmd) {
    if (!validate_command(cmd)) return NULL;

    FILE *pipe = popen(cmd, "r");
    if (!pipe) return NULL;

    char *output = malloc(MAX_OUTPUT_LEN);
    if (!output) {
        pclose(pipe);
        return NULL;
    }

    size_t total = 0;
    size_t n;
    while (total < MAX_OUTPUT_LEN - 1 && (n = fread(output + total, 1, MAX_OUTPUT_LEN - 1 - total, pipe)) > 0) {
        total += n;
    }
    output[total] = '\0';

    pclose(pipe);
    return output;
}

int main(void) {
    char cmd[MAX_CMD_LEN];

    if (!fgets(cmd, sizeof(cmd), stdin)) {
        return 1;
    }

    size_t len = strlen(cmd);
    if (len > 0 && cmd[len-1] == '\n') cmd[--len] = '\0';

    if (!validate_command(cmd)) {
        fprintf(stderr, "Invalid command\n");
        return 1;
    }

    char *result = execute_command(cmd);
    if (!result) {
        fprintf(stderr, "Execution failed\n");
        return 1;
    }

    fputs(result, stdout);
    free(result);
    return 0;
}