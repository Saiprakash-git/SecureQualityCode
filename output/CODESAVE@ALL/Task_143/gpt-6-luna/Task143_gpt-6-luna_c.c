#include <stdio.h>
#include <stdlib.h>

static int compare_ints(const void *a, const void *b)
{
    int x = *(const int *)a;
    int y = *(const int *)b;
    return (x > y) - (x < y);
}

static void sort_ascending(int values[], size_t count)
{
    qsort(values, count, sizeof(values[0]), compare_ints);
}

int main(void)
{
    size_t count;

    if (scanf("%zu", &count) != 1)
        return 1;

    int *values = malloc(count * sizeof(*values));
    if (count != 0 && values == NULL)
        return 1;

    for (size_t i = 0; i < count; ++i) {
        if (scanf("%d", &values[i]) != 1) {
            free(values);
            return 1;
        }
    }

    sort_ascending(values, count);

    for (size_t i = 0; i < count; ++i)
        printf("%s%d", i == 0 ? "" : " ", values[i]);
    putchar('\n');

    free(values);
    return 0;
}