#include <stdlib.h>
#include <string.h>

char* highestScoringWord(const char* str) {
    if (!str) {
        return NULL;
    }

    int max_score = -1;
    const char* best_word_start = NULL;
    size_t best_word_len = 0;

    const char* p = str;
    while (*p) {
        while (*p == ' ') {
            p++;
        }
        if (!*p) {
            break;
        }

        const char* word_start = p;
        int current_score = 0;

        while (*p && *p != ' ') {
            current_score += (*p - 'a' + 1);
            p++;
        }

        size_t word_len = (size_t)(p - word_start);

        if (current_score > max_score) {
            max_score = current_score;
            best_word_start = word_start;
            best_word_len = word_len;
        }
    }

    if (!best_word_start) {
        char* empty = (char*)malloc(1);
        if (empty) {
            empty[0] = '\0';
        }
        return empty;
    }

    char* result = (char*)malloc(best_word_len + 1);
    if (!result) {
        return NULL;
    }

    memcpy(result, best_word_start, best_word_len);
    result[best_word_len] = '\0';

    return result;
}