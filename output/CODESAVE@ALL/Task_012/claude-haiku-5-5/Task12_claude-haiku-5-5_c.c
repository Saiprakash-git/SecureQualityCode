/* Returns the highest scoring word from the input string.
   Caller must free the returned string. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

char *high(const char *s) {
    size_t len = strlen(s);
    size_t best_start = 0, best_len = 0;
    int best_score = -1;
    size_t i = 0;

    while (i < len) {
        while (i < len && isspace((unsigned char)s[i])) i++;
        if (i >= len) break;
        size_t start = i;
        int score = 0;
        while (i < len && !isspace((unsigned char)s[i])) {
            if (s[i] >= 'a' && s[i] <= 'z')
                score += s[i] - 'a' + 1;
            i++;
        }
        size_t wlen = i - start;
        if (score > best_score) {
            best_score = score;
            best_start = start;
            best_len = wlen;
        }
    }

    char *result = (char *)malloc(best_len + 1);
    if (!result) return NULL;
    memcpy(result, s + best_start, best_len);
    result[best_len] = '\0';
    return result;
}

#ifdef TEST_HIGH
int main(void) {
    char *r = high("man i need a taxi up to ubud");
    printf("%s\n", r);
    free(r);
    return 0;
}
#endif