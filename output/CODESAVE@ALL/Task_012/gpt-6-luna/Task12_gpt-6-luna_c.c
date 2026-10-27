#include <stdlib.h>
#include <string.h>

char *high(const char *str)
{
    const char *best_start = str;
    size_t best_length = 0;
    int best_score = -1;

    while (*str) {
        while (*str == ' ')
            ++str;

        const char *word_start = str;
        int score = 0;

        while (*str && *str != ' ') {
            score += *str - 'a' + 1;
            ++str;
        }

        size_t length = (size_t)(str - word_start);
        if (length > 0 && score > best_score) {
            best_score = score;
            best_start = word_start;
            best_length = length;
        }
    }

    char *result = malloc(best_length + 1);
    if (!result)
        return NULL;

    memcpy(result, best_start, best_length);
    result[best_length] = '\0';
    return result;
}