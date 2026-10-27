#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>

static int line_matches(const char *line, regex_t *regex)
{
    return regexec(regex, line, 0, NULL, 0) == 0;
}

static void process_file(const char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        perror(filename);
        return;
    }

    regex_t regex;
    int rc = regcomp(&regex, "^[0-9]+.*\\.$", REG_EXTENDED);
    if (rc != 0) {
        char errbuf[256];
        regerror(rc, &regex, errbuf, sizeof(errbuf));
        fprintf(stderr, "Regex error: %s\n", errbuf);
        fclose(fp);
        return;
    }

    char line[4096];
    while (fgets(line, sizeof(line), fp) != NULL) {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }

        if (line_matches(line, &regex)) {
            printf("%s\n", line);
        }
    }

    regfree(&regex);
    fclose(fp);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return 1;
    }

    process_file(argv[1]);
    return 0;
}