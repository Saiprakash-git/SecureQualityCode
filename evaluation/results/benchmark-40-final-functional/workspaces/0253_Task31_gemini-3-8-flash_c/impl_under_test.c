#include <stdlib.h>
#include <string.h>

char *dna_strand(const char *dna) {
    if (!dna) {
        return NULL;
    }

    size_t len = strlen(dna);
    char *complement = (char *)malloc(len + 1);
    if (!complement) {
        return NULL;
    }

    for (size_t i = 0; i < len; i++) {
        switch (dna[i]) {
            case 'A':
                complement[i] = 'T';
                break;
            case 'T':
                complement[i] = 'A';
                break;
            case 'C':
                complement[i] = 'G';
                break;
            case 'G':
                complement[i] = 'C';
                break;
            default:
                complement[i] = dna[i];
                break;
        }
    }
    complement[len] = '\0';

    return complement;
}