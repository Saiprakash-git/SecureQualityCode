#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char **build_tower(unsigned n) {
    if (n == 0) {
        return NULL;
    }

    char **tower = (char **)malloc(n * sizeof(char *));
    if (!tower) {
        return NULL;
    }

    unsigned width = 2 * n - 1;

    for (unsigned i = 0; i < n; i++) {
        tower[i] = (char *)malloc((width + 1) * sizeof(char));
        if (!tower[i]) {
            for (unsigned j = 0; j < i; j++) {
                free(tower[j]);
            }
            free(tower);
            return NULL;
        }

        unsigned spaces = n - 1 - i;
        unsigned stars = 2 * i + 1;

        memset(tower[i], ' ', spaces);
        memset(tower[i] + spaces, '*', stars);
        memset(tower[i] + spaces + stars, ' ', spaces);
        tower[i][width] = '\0';
    }

    return tower;
}

void free_tower(char **tower, unsigned n) {
    if (!tower) {
        return;
    }
    for (unsigned i = 0; i < n; i++) {
        free(tower[i]);
    }
    free(tower);
}

int main(void) {
    unsigned floors = 6;
    char **tower = build_tower(floors);

    if (tower) {
        printf("[\n");
        for (unsigned i = 0; i < floors; i++) {
            printf("  \"%s\"%s\n", tower[i], (i + 1 < floors) ? "," : "");
        }
        printf("]\n");

        free_tower(tower, floors);
    }

    return 0;
}