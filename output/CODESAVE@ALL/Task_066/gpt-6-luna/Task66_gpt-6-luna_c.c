#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define INPUT_SIZE 4096
#define MAX_ARGS 64

static int allowed_character(unsigned char c)
{
    if (isalnum(c) || c == ' ' || c == '\t')
        return 1;

    switch (c) {
    case '.':
    case '_':
    case '/':
    case '-':
    case '!':
    case '?':
    case ',':
    case ':':
    case '@':
    case '+':
    case '=':
        return 1;
    default:
        return 0;
    }
}

static int parse_command(char *input, char **argv, size_t *argc)
{
    size_t i;
    char *saveptr = NULL;
    char *token;

    for (i = 0; input[i] != '\0'; ++i) {
        if (!allowed_character((unsigned char)input[i]))
            return -1;
    }

    *argc = 0;
    token = strtok_r(input, " \t", &saveptr);
    while (token != NULL) {
        if (*argc >= MAX_ARGS)
            return -1;
        argv[(*argc)++] = token;
        token = strtok_r(NULL, " \t", &saveptr);
    }

    argv[*argc] = NULL;
    return *argc == 0 ? -1 : 0;
}

static const char *command_path(const char *name, size_t argc)
{
    if (strcmp(name, "pwd") == 0 && argc == 1)
        return "/bin/pwd";
    if (strcmp(name, "date") == 0 && argc == 1)
        return "/bin/date";
    if (strcmp(name, "ls") == 0 && argc == 1)
        return "/bin/ls";
    if (strcmp(name, "whoami") == 0 && argc == 1)
        return "/usr/bin/whoami";
    return NULL;
}

static int run_echo(char **argv, size_t argc)
{
    size_t i;

    for (i = 1; i < argc; ++i) {
        if (i > 1 && putchar(' ') == EOF)
            return 1;
        if (fputs(argv[i], stdout) == EOF)
            return 1;
    }

    if (putchar('\n') == EOF || fflush(stdout) == EOF)
        return 1;
    return 0;
}

static int execute_command(char **argv, size_t argc)
{
    const char *path;
    pid_t child;
    int status;

    if (strcmp(argv[0], "echo") == 0)
        return run_echo(argv, argc);

    path = command_path(argv[0], argc);
    if (path == NULL) {
        fprintf(stderr, "Rejected: command is not permitted or has invalid arguments.\n");
        return 2;
    }

    child = fork();
    if (child < 0) {
        perror("fork");
        return 1;
    }

    if (child == 0) {
        execv(path, argv);
        perror("execv");
        _exit(127);
    }

    do {
        if (waitpid(child, &status, 0) >= 0)
            break;
    } while (errno == EINTR);

    if (errno != EINTR && status == 0) {
        /* The child exited successfully. */
    } else if (errno != EINTR && !WIFEXITED(status) && !WIFSIGNALED(status)) {
        perror("waitpid");
        return 1;
    }

    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    if (WIFSIGNALED(status))
        return 128 + WTERMSIG(status);
    return 1;
}

int main(void)
{
    char input[INPUT_SIZE];
    char *argv[MAX_ARGS + 1];
    size_t argc;
    size_t length;

    fputs("command> ", stdout);
    fflush(stdout);

    if (fgets(input, sizeof(input), stdin) == NULL) {
        if (ferror(stdin))
            perror("fgets");
        return 1;
    }

    length = strlen(input);
    if (length > 0 && input[length - 1] == '\n') {
        input[--length] = '\0';
    } else if (!feof(stdin)) {
        int c;
        while ((c = getchar()) != '\n' && c != EOF)
            ;
        fprintf(stderr, "Rejected: input is too long.\n");
        return 2;
    }

    if (length > 0 && input[length - 1] == '\r')
        input[--length] = '\0';

    if (parse_command(input, argv, &argc) != 0) {
        fprintf(stderr, "Rejected: invalid command format or characters.\n");
        return 2;
    }

    return execute_command(argv, argc);
}