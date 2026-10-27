#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

char *likes(size_t n, const char *const names[]) {
    int len = 0;
    char *result = NULL;

    switch (n) {
        case 0:
            len = snprintf(NULL, 0, "no one likes this");
            result = (char *)malloc(len + 1);
            if (result) {
                snprintf(result, len + 1, "no one likes this");
            }
            break;
        case 1:
            len = snprintf(NULL, 0, "%s likes this", names[0]);
            result = (char *)malloc(len + 1);
            if (result) {
                snprintf(result, len + 1, "%s likes this", names[0]);
            }
            break;
        case 2:
            len = snprintf(NULL, 0, "%s and %s like this", names[0], names[1]);
            result = (char *)malloc(len + 1);
            if (result) {
                snprintf(result, len + 1, "%s and %s like this", names[0], names[1]);
            }
            break;
        case 3:
            len = snprintf(NULL, 0, "%s, %s and %s like this", names[0], names[1], names[2]);
            result = (char *)malloc(len + 1);
            if (result) {
                snprintf(result, len + 1, "%s, %s and %s like this", names[0], names[1], names[2]);
            }
            break;
        default:
            len = snprintf(NULL, 0, "%s, %s and %zu others like this", names[0], names[1], n - 2);
            result = (char *)malloc(len + 1);
            if (result) {
                snprintf(result, len + 1, "%s, %s and %zu others like this", names[0], names[1], n - 2);
            }
            break;
    }

    return result;
}

int main(void) {
    const char *names0[] = { NULL };
    const char *names1[] = { "Peter" };
    const char *names2[] = { "Jacob", "Alex" };
    const char *names3[] = { "Max", "John", "Mark" };
    const char *names4[] = { "Alex", "Jacob", "Mark", "Max" };

    char *s0 = likes(0, names0);
    char *s1 = likes(1, names1);
    char *s2 = likes(2, names2);
    char *s3 = likes(3, names3);
    char *s4 = likes(4, names4);

    if (s0) { printf("%s\n", s0); free(s0); }
    if (s1) { printf("%s\n", s1); free(s1); }
    if (s2) { printf("%s\n", s2); free(s2); }
    if (s3) { printf("%s\n", s3); free(s3); }
    if (s4) { printf("%s\n", s4); free(s4); }

    return 0;
}