#include <stdlib.h>
#include <string.h>

char *dna_strand(const char *dna)
{
    if (dna == NULL)
        return NULL;

    size_t len = strlen(dna);
    char *result = malloc(len + 1);

    if (result == NULL)
        return NULL;

    for (size_t i = 0; i < len; ++i)
    {
        switch (dna[i])
        {
            case 'A':
                result[i] = 'T';
                break;
            case 'T':
                result[i] = 'A';
                break;
            case 'C':
                result[i] = 'G';
                break;
            case 'G':
                result[i] = 'C';
                break;
            default:
                result[i] = dna[i];
                break;
        }
    }

    result[len] = '\0';
    return result;
}