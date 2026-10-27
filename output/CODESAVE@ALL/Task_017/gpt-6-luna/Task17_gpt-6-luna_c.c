#include <ctype.h>
#include <stdlib.h>
#include <string.h>

char *to_jaden_case(const char *jaden)
{
    if (jaden == NULL || jaden[0] == '\0')
        return NULL;

    size_t length = strlen(jaden);
    char *result = malloc(length + 1);
    if (result == NULL)
        return NULL;

    int at_word_start = 1;
    for (size_t i = 0; i < length; ++i) {
        unsigned char c = (unsigned char)jaden[i];
        result[i] = at_word_start ? (char)toupper(c) : (char)c;
        at_word_start = isspace(c);
    }

    result[length] = '\0';
    return result;
}