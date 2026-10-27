#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>

static void trim_newline(char *str)
{
    if (!str) {
        return;
    }
    size_t len = strlen(str);
    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[len - 1] = '\0';
        len--;
    }
}

int process_file_regex(const char *filename, const char *pattern)
{
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Error opening file");
        return -1;
    }

    regex_t regex;
    int ret = regcomp(&regex, pattern, REG_EXTENDED);
    if (ret != 0) {
        char errbuf[256];
        regerror(ret, &regex, errbuf, sizeof(errbuf));
        fprintf(stderr, "Regex compilation error: %s\n", errbuf);
        fclose(fp);
        return -1;
    }

    char *line = NULL;
    size_t cap = 0;
    ssize_t len;
    int match_count = 0;

    while ((len = getline(&line, &cap, fp)) != -1) {
        trim_newline(line);
        if (regexec(&regex, line, 0, NULL, 0) == 0) {
            printf("%s\n", line);
            match_count++;
        }
    }

    free(line);
    regfree(&regex);
    fclose(fp);

    return match_count;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }

    /* Pattern: Starts with one or more digits and ends with a literal dot */
    const char *pattern = "^[0-9]+.*\\.$";

    int matches = process_file_regex(argv[1], pattern);
    if (matches < 0) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}