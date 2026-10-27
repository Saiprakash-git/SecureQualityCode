#include <stdio.h>
#include <stdlib.h>

static int compare_int(const void *a, const void *b)
{
    int x = *(const int *)a;
    int y = *(const int *)b;

    if (x < y)
        return -1;
    if (x > y)
        return 1;
    return 0;
}

void sort_integers(int *arr, size_t n)
{
    if (arr != NULL && n > 1)
        qsort(arr, n, sizeof(int), compare_int);
}

int main(void)
{
    size_t n;
    int *arr;
    size_t i;

    if (scanf("%zu", &n) != 1 || n == 0)
        return 0;

    arr = malloc(n * sizeof(int));
    if (arr == NULL)
        return 1;

    for (i = 0; i < n; ++i)
    {
        if (scanf("%d", &arr[i]) != 1)
        {
            free(arr);
            return 1;
        }
    }

    sort_integers(arr, n);

    for (i = 0; i < n; ++i)
    {
        if (i > 0)
            printf(" ");
        printf("%d", arr[i]);
    }

    printf("\n");

    free(arr);
    return 0;
}