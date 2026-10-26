#include <stdio.h>
#include <ctype.h>
#include <string.h>

int is_pangram(const char *s) {
    int seen[26] = {0};
    int count = 0;
    for (size_t i = 0; s[i] != '\0'; i++) {
        unsigned char c = (unsigned char)s[i];
        if (isalpha(c)) {
            int idx = tolower(c) - 'a';
            if (idx >= 0 && idx < 26 && !seen[idx]) {
                seen[idx] = 1;
                count++;
            }
        }
    }
    return count == 26;
}

int main(void) {
    char buffer[4096];
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        return 0;
    }
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    }
    printf("%s\n", is_pangram(buffer) ? "True" : "False");
    return 0;
}