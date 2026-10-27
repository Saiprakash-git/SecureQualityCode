#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_CMD_LEN 1024
#define MAX_OUT_LEN 65536

typedef enum {
    VALID_OK = 0,
    VALID_EMPTY,
    VALID_TOO_LONG,
    VALID_BAD_CHAR,
    VALID_BAD_START,
    VALID_BAD_END,
    VALID_UNBALANCED_QUOTES,
    VALID_BLOCKED_CONTENT
} ValidationResult;

static const char *validation_message(ValidationResult r) {
    switch (r) {
        case VALID_OK: return "ok";
        case VALID_EMPTY: return "command is empty";
        case VALID_TOO_LONG: return "command exceeds maximum length";
        case VALID_BAD_CHAR: return "command contains non-printable or disallowed characters";
        case VALID_BAD_START: return "command starts with an invalid character";
        case VALID_BAD_END: return "command ends with an incomplete operator or escape";
        case VALID_UNBALANCED_QUOTES: return "command has unbalanced quotes";
        case VALID_BLOCKED_CONTENT: return "command contains a blocked or dangerous pattern";
        default: return "unknown validation error";
    }
}

static void trim_newline(char *s) {
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r')) {
        s[--n] = '\0';
    }
}

static int is_blank(const char *s) {
    while (*s) {
        if (!isspace((unsigned char)*s)) return 0;
        s++;
    }
    return 1;
}

static ValidationResult validate_format(const char *cmd) {
    size_t len = strlen(cmd);

    if (is_blank(cmd)) return VALID_EMPTY;
    if (len > MAX_CMD_LEN) return VALID_TOO_LONG;

    for (const char *p = cmd; *p; p++) {
        unsigned char c = (unsigned char)*p;
        if (c == '\t' || !isprint(c)) return VALID_BAD_CHAR;
    }

    const char *start = cmd;
    while (isspace((unsigned char)*start)) start++;
    if (*start == ';' || *start == '|' || *start == '&') return VALID_BAD_START;

    size_t end = strlen(start);
    while (end > 0 && isspace((unsigned char)start[end - 1])) end--;
    if (end > 0) {
        char last = start[end - 1];
        if (last == '|' || last == '&' || last == '\\') return VALID_BAD_END;
    }

    int in_single = 0, in_double = 0;
    for (const char *p = start; *p; p++) {
        if (in_single) {
            if (*p == '\'') in_single = 0;
        } else if (*p == '\\' && !in_double) {
            if (p[1] == '\0') return VALID_BAD_END;
            p++;
        } else if (*p == '"') {
            in_double = !in_double;
        } else if (*p == '\'' && !in_double) {
            in_single = 1;
        }
    }
    if (in_single || in_double) return VALID_UNBALANCED_QUOTES;

    return VALID_OK;
}

static ValidationResult validate_content(const char *cmd) {
    static const char *blocked[] = {
        "rm -rf /",
        "rm -rf ~",
        "rm -rf *",
        "rm -fr /",
        "mkfs",
        "dd if=",
        ":(){",
        "> /dev/sd",
        "> /dev/hd",
        "> /dev/nvme",
        "chmod -R 777 /",
        "chown -R",
        "/etc/shadow",
        "shutdown",
        "reboot",
        "poweroff",
        "halt",
        "init 0",
        "init 6",
        "sudo ",
        "su -",
        "| sh",
        "| bash",
        "|sh",
        "|bash",
        "curl | ",
        "wget | ",
        NULL
    };

    for (int i = 0; blocked[i] != NULL; i++) {
        if (strstr(cmd, blocked[i]) != NULL) {
            return VALID_BLOCKED_CONTENT;
        }
    }
    return VALID_OK;
}

static ValidationResult validate_command(const char *cmd) {
    ValidationResult r = validate_format(cmd);
    if (r != VALID_OK) return r;
    return validate_content(cmd);
}

static int execute_command(const char *cmd, char *output, size_t outsz, int *exit_status) {
    int fd[2];
    if (pipe(fd) == -1) {
        return -1;
    }

    fflush(stdout);
    pid_t pid = fork();
    if (pid == -1) {
        close(fd[0]);
        close(fd[1]);
        return -1;
    }

    if (pid == 0) {
        close(fd[0]);
        if (dup2(fd[1], STDOUT_FILENO) == -1 || dup2(fd[1], STDERR_FILENO) == -1) {
            _exit(127);
        }
        close(fd[1]);
        execl("/bin/bash", "bash", "-c", cmd, (char *)NULL);
        _exit(127);
    }

    close(fd[1]);

    size_t used = 0;
    if (outsz > 0) output[0] = '\0';
    char buf[4096];
    ssize_t n;
    while ((n = read(fd[0], buf, sizeof(buf))) != 0) {
        if (n < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (outsz > 1) {
            size_t space = outsz - 1 - used;
            size_t copy = (size_t)n < space ? (size_t)n : space;
            memcpy(output + used, buf, copy);
            used += copy;
            output[used] = '\0';
        }
    }
    close(fd[0]);

    int status = 0;
    while (waitpid(pid, &status, 0) == -1) {
        if (errno != EINTR) return -1;
    }

    if (WIFEXITED(status)) {
        *exit_status = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        *exit_status = 128 + WTERMSIG(status);
    } else {
        *exit_status = -1;
    }
    return 0;
}

static int read_command(char *buf, size_t size) {
    if (fgets(buf, (int)size, stdin) == NULL) {
        return -1;
    }
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        trim_newline(buf);
        return 0;
    }
    if (!feof(stdin)) {
        int c;
        while ((c = getchar()) != '\n' && c != EOF) {
        }
        return 1;
    }
    return 0;
}

int main(void) {
    char command[MAX_CMD_LEN + 2];
    static char output[MAX_OUT_LEN];

    printf("Enter a bash command: ");
    fflush(stdout);

    int rc = read_command(command, sizeof(command));
    if (rc < 0) {
        fprintf(stderr, "Error: failed to read input.\n");
        return EXIT_FAILURE;
    }
    if (rc > 0) {
        fprintf(stderr, "Error: %s.\n", validation_message(VALID_TOO_LONG));
        return EXIT_FAILURE;
    }

    ValidationResult vr = validate_command(command);
    if (vr != VALID_OK) {
        fprintf(stderr, "Validation failed: %s.\n", validation_message(vr));
        return EXIT_FAILURE;
    }

    int exit_status = 0;
    if (execute_command(command, output, sizeof(output), &exit_status) != 0) {
        fprintf(stderr, "Error: failed to execute command.\n");
        return EXIT_FAILURE;
    }

    printf("--- Output ---\n");
    fputs(output, stdout);
    if (output[0] != '\0' && output[strlen(output) - 1] != '\n') {
        putchar('\n');
    }
    printf("--- Exit status: %d ---\n", exit_status);

    return exit_status == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}