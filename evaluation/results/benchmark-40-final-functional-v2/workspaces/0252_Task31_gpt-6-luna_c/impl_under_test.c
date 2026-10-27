#include <stdlib.h>
#include <string.h>

char *dna_strand(const char *dna)
{
    size_t length = strlen(dna);
    char *complement = malloc(length + 1);

    if (complement == NULL)
        return NULL;

    for (size_t i = 0; i < length; ++i) {
        switch (dna[i]) {
            case 'A': complement[i] = 'T'; break;
            case 'T': complement[i] = 'A'; break;
            case 'C': complement[i] = 'G'; break;
            case 'G': complement[i] = 'C'; break;
            default:  complement[i] = dna[i]; break;
        }
    }

    complement[length] = '\0';
    return complement;
}