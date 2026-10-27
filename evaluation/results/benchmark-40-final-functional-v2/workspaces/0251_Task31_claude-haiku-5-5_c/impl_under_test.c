#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *dna_strand(const char *dna) {
    size_t len = strlen(dna);
    char *out = malloc(len + 1);
    if (out == NULL) {
        return NULL;
    }
    for (size_t i = 0; i < len; i++) {
        switch (dna[i]) {
            case 'A': out[i] = 'T'; break;
            case 'T': out[i] = 'A'; break;
            case 'C': out[i] = 'G'; break;
            case 'G': out[i] = 'C'; break;
            default:  out[i] = dna[i]; break;
        }
    }
    out[len] = '\0';
    return out;
}

int main(void) {
    const char *samples[] = { "ATTGC", "GTAT" };
    size_t count = sizeof(samples) / sizeof(samples[0]);
    for (size_t i = 0; i < count; i++) {
        char *result = dna_strand(samples[i]);
        if (result != NULL) {
            printf("%s --> %s\n", samples[i], result);
            free(result);
        }
    }
    return 0;
}