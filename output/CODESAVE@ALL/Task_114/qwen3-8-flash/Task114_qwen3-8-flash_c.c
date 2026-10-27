#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

size_t my_strlen(const char *s)
{
    size_t len = 0;

    if (s == NULL)
        return 0;

    while (s[len] != '\0')
        len++;

    return len;
}

char *my_strcpy(char *dest, const char *src)
{
    if (dest == NULL)
        return NULL;

    if (src == NULL) {
        dest[0] = '\0';
        return dest;
    }

    char *d = dest;
    while (*src != '\0') {
        *d++ = *src++;
    }
    *d = '\0';

    return dest;
}

char *my_strcat(char *dest, const char *src)
{
    if (dest == NULL)
        return NULL;

    char *end = dest;
    while (*end != '\0')
        end++;

    if (src == NULL)
        return dest;

    while (*src != '\0') {
        *end++ = *src++;
    }
    *end = '\0';

    return dest;
}

char *my_strdup(const char *s)
{
    size_t len = my_strlen(s);
    char *res = malloc(len + 1);

    if (res == NULL)
        return NULL;

    my_strcpy(res, s);
    return res;
}

char *concatenate(const char *a, const char *b