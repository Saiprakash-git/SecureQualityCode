#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* highest_scoring_word(const char* input) {
    if (!input) return NULL;
    int max_score = -1;
    char* best_word = NULL;
    const char* p = input;
    while (*p) {
        while (*p == ' ') p++;
        if (!*p) break;
        const char* start = p;
        while (*p && *p != ' ') p++;
        int score = 0;
        for (const char* c = start; c < p; c++) {
            score += (*c - 'a' + 1);
        }
        if (score > max_score) {
            max_score = score;
            free(best_word);
            int len = p - start;
            best_word = malloc(len + 1);
            if (best_word) {
                memcpy(best_word, start, len);
                best_word[len] = '\0';
            }
        }
    }
    return best_word;
}

int main() {
    char* result = highest_scoring_word("abad zzz");
    if (result) {
        printf("%s\n", result);
        free(result);
    }
    return 0;
}