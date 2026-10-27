#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>

#define MAX_COMMAND_LENGTH 4096

int is_blank_string(const char *str) {
    if (str == NULL) {
        return 1;
    }
    while (*str) {
        if (!isspace((unsigned char)*str)) {
            return 0;
        }
        str++;
    }
    return 1;
}

int has_invalid_characters(const char *str) {
    if (str == NULL) {
        return 1;
    }
    while (*str) {
        if (!isprint((unsigned char)*str) && !isspace((unsigned char)*str)) {
            return 1;
        }
        str++;
    }
    return 0;
}

int check_bash_syntax(const char *cmd) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        return -1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        close(pipefd[0]);
        close(pipefd[1]);
        return -1;
    }

    if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);
        execl("/bin/bash", "bash", "-n", "-c", cmd, (char *)NULL);
        _exit(127);
    }

    close(pipefd[1]);
    close(pipefd[0]);

    int status = 0;
    if (waitpid(pid, &status, 0) == -1) {
        return -1;
    }

    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        return 0;
    }

    return 1;
}

int validate_command(const char *cmd, char *error_buffer, size_t error_buf_len) {
    if (cmd == NULL || strlen(cmd) == 0) {
        snprintf(error_buffer, error_buf_len, "Error: Command cannot be empty.");
        return 0;
    }

    if (strlen(cmd) >= MAX_COMMAND_LENGTH) {
        snprintf(error_buffer, error_buf_len, "Error: Command exceeds maximum allowed length (%d bytes).", MAX_COMMAND_LENGTH);
        return 0;
    }

    if (is_blank_string(cmd)) {
        snprintf(error_buffer, error_buf_len, "Error: Command cannot consist solely of whitespace.");
        return 0;
    }

    if (has_invalid_characters(cmd)) {
        snprintf(error_buffer, error_buf_len, "Error: Command contains non-printable or unsupported control characters.");
        return 0;
    }

    if (check_bash_syntax(cmd) != 0) {
        snprintf(error_buffer, error_buf_len, "Error: Command failed Bash syntax validation.");
        return 0;
    }

    return 1;
}

char *execute_command(const char *cmd, int *exit_code) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        return NULL;
    }

    pid_t pid = fork();
    if (pid == -1) {
        close(pipefd[0]);
        close(pipefd[1]);
        return NULL;
    }

    if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);
        execl("/bin/bash", "bash", "-c", cmd, (char *)NULL);
        _exit(127);
    }

    close(pipefd[1]);

    size_t capacity = 1024;
    size_t length = 0;
    char *output = malloc(capacity);
    if (output == NULL) {
        close(pipefd[0]);
        waitpid(pid, NULL, 0);
        return NULL;
    }

    char buffer[256];
    ssize_t bytes_read;
    while ((bytes_read = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
        if (length + (size_t)bytes_read + 1 > capacity) {
            capacity = (length + (size_t)bytes_read + 1) * 2;
            char *new_output = realloc(output, capacity);
            if (new_output == NULL) {
                free(output);
                close(pipefd[0]);
                waitpid(pid, NULL, 0);
                return NULL;
            }
            output = new_output;
        }
        memcpy(output + length, buffer, bytes_read);
        length += bytes_read;
    }
    output[length] = '\0';
    close(pipefd[0]);

    int status = 0;
    waitpid(pid, &status, 0);

    if (exit_code != NULL) {
        if (WIFEXITED(status)) {
            *exit_code = WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            *exit_code = 128 + WTERMSIG(status);
        } else {
            *exit_code = -1;
        }
    }

    return output;
}

int read_line(char *buffer, size_t size, FILE *stream) {
    if (fgets(buffer, (int)size, stream) == NULL) {
        return 0;
    }
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    }
    return 1;
}

int main(void) {
    char input[MAX_COMMAND_LENGTH];
    char error_msg[256];

    printf("Enter a bash command to execute: ");
    fflush(stdout);

    if (!read_line(input, sizeof(input), stdin)) {
        fprintf(stderr, "Failed to read input.\n");
        return EXIT_FAILURE;
    }

    if (!validate_command(input, error_msg, sizeof(error_msg))) {
        fprintf(stderr, "Validation Failure: %s\n", error_msg);
        return EXIT_FAILURE;
    }

    int exit_code = 0;
    char *output = execute_command(input, &exit_code);
    if (output == NULL) {
        fprintf(stderr, "Execution Error: Failed to execute command.\n");
        return EXIT_FAILURE;
    }

    printf("\n--- Command Output ---\n%s", output);
    if (strlen(output) > 0 && output[strlen(output) - 1] != '\n') {
        printf("\n");
    }
    printf("--- Exit Code: %d ---\n", exit_code);

    free(output);
    return exit_code;
}