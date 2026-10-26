#include <stdlib.h>
#include <string.h>

char **build_tower(int floors)
{
    if (floors < 0)
        return NULL;

    char **tower = malloc((size_t)(floors + 1) * sizeof(char *));
    if (!tower)
        return NULL;

    tower[floors] = NULL;

    if (floors == 0)
        return tower;

    int width = 2 * floors - 1;

    for (int i = 0; i < floors; ++i)
    {
        char *line = malloc((size_t)width + 1);
        if (!line)
        {
            for (int j = 0; j < i; ++j)
                free(tower[j]);
            free(tower);
            return NULL;
        }

        int spaces = floors - i - 1;
        int stars = 2 * i + 1;

        memset(line, ' ', (size_t)spaces);
        memset(line + spaces, '*', (size_t)stars);
        memset(line + spaces + stars, ' ', (size_t)(width - spaces - stars));

        line[width] = '\0';
        tower[i] = line;
    }

    return tower;
}

void free_tower(char **tower)
{
    if (!tower)
        return;

    for (int i = 0; tower[i] != NULL; ++i)
        free(tower[i]);

    free(tower);
}