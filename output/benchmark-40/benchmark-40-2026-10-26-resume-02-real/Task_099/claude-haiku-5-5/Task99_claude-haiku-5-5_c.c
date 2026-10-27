#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>

#define PATTERN "^[0-9]+.*\\.$"

int compile_pattern(regex_t *re, const char *pattern) {
    return regcomp(re, pattern, REG_EXTENDED | REG_NOSUB) == 0;
}

int line_matches(const regex_t *re, const char *line) {
    return regexec(re, line, 0, NULL, 0) == 0;
}

void strip_newline(char *line) {
    size_t len = strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
        line[--len] = '\0';
    }
}

long process_stream(FILE *in, FILE *out, const regex_t *re) {
    char *buf = NULL;
    size_t cap = 0;
    ssize_t n;
    long count = 0;

    while ((n = getline(&buf, &cap, in)) != -1) {
        strip_newline(buf);
        if (line_matches(re, buf)) {
            fprintf(out, "%s\n", buf);
            count++;
        }
    }

    free(buf);
    return count;
}

int main(int argc, char *argv[]) {
    regex_t re;
    FILE *in = stdin;
    long count;

    if (!compile_pattern(&re, PATTERN)) {
        fprintf(stderr, "Failed to compile regular expression.\n");
        return EXIT_FAILURE;
    }

    if (argc > 1) {
        in = fopen(argv[1], "r");
        if (!in) {
            perror("fopen");
            regfree(&re);
            return EXIT_FAILURE;
        }
    }

    count = process_stream(in, stdout, &re);
    fprintf(stderr, "Matched lines: %ld\n", count);

    if (in != stdin) {
        fclose(in);
    }
    regfree(&re);
    return EXIT_SUCCESS;
}