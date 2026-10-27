#include <stdlib.h>

char **tower_builder(unsigned n_floors)
{
    if (n_floors == 0)
        return NULL;

    size_t width = 2 * (size_t)n_floors - 1;
    char **tower = malloc((size_t)n_floors * sizeof(*tower));
    if (tower == NULL)
        return NULL;

    for (unsigned row = 0; row < n_floors; ++row) {
        tower[row] = malloc(width + 1);
        if (tower[row] == NULL) {
            for (unsigned i = 0; i < row; ++i)
                free(tower[i]);
            free(tower);
            return NULL;
        }

        size_t stars = 2 * (size_t)row + 1;
        size_t padding = ((size_t)n_floors - 1 - row);
        for (size_t i = 0; i < width; ++i)
            tower[row][i] = (i >= padding && i < padding + stars) ? '*' : ' ';
        tower[row][width] = '\0';
    }

    return tower;
}