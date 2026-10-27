#include <stdio.h>
#include <stdlib.h>

char *likes(const char *names[], size_t count)
{
    int length;
    char *result;

    if (count == 0) {
        length = snprintf(NULL, 0, "no one likes this");
    } else if (count == 1) {
        length = snprintf(NULL, 0, "%s likes this", names[0]);
    } else if (count == 2) {
        length = snprintf(NULL, 0, "%s and %s like this", names[0], names[1]);
    } else if (count == 3) {
        length = snprintf(NULL, 0, "%s, %s and %s like this",
                          names[0], names[1], names[2]);
    } else {
        length = snprintf(NULL, 0, "%s, %s and %zu others like this",
                          names[0], names[1], count - 2);
    }

    if (length < 0) {
        return NULL;
    }

    result = malloc((size_t)length + 1);
    if (result == NULL) {
        return NULL;
    }

    if (count == 0) {
        snprintf(result, (size_t)length + 1, "no one likes this");
    } else if (count == 1) {
        snprintf(result, (size_t)length + 1, "%s likes this", names[0]);
    } else if (count == 2) {
        snprintf(result, (size_t)length + 1, "%s and %s like this",
                 names[0], names[1]);
    } else if (count == 3) {
        snprintf(result, (size_t)length + 1, "%s, %s and %s like this",
                 names[0], names[1], names[2]);
    } else {
        snprintf(result, (size_t)length + 1, "%s, %s and %zu others like this",
                 names[0], names[1], count - 2);
    }

    return result;
}