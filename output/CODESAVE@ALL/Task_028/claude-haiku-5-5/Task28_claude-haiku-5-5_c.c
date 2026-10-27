#include <stdio.h>
#include <stddef.h>

char *likes(const char *const names[], size_t n, char *buf, size_t bufsize)
{
    if (bufsize == 0) return buf;

    if (n == 0) {
        snprintf(buf, bufsize, "no one likes this");
    } else if (n == 1) {
        snprintf(buf, bufsize, "%s likes this", names[0]);
    } else if (n == 2) {
        snprintf(buf, bufsize, "%s and %s like this", names[0], names[1]);
    } else if (n == 3) {
        snprintf(buf, bufsize, "%s, %s and %s like this", names[0], names[1], names[2]);
    } else {
        snprintf(buf, bufsize, "%s, %s and %zu others like this", names[0], names[1], n - 2);
    }

    return buf;
}

int main(void)
{
    char buf[256];

    const char *t0[] = { NULL };
    const char *t1[] = { "Peter" };
    const char *t2[] = { "Jacob", "Alex" };
    const char *t3[] = { "Max", "John", "Mark" };
    const char *t4[] = { "Alex", "Jacob", "Mark", "Max" };

    printf("%s\n", likes(t0, 0, buf, sizeof buf));
    printf("%s\n", likes(t1, 1, buf, sizeof buf));
    printf("%s\n", likes(t2, 2, buf, sizeof buf));
    printf("%s\n", likes(t3, 3, buf, sizeof buf));
    printf("%s\n", likes(t4, 4, buf, sizeof buf));

    return 0;
}