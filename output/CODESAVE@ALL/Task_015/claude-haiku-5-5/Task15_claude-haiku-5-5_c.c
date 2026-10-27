#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char **build_tower(int nFloors) {
    if (nFloors <= 0) return NULL;

    int width = 2 * nFloors - 1;
    char **tower = malloc(sizeof(char *) * nFloors);
    if (!tower) return NULL;

    for (int i = 0; i < nFloors; i++) {
        tower[i] = malloc(width + 1);
        if (!tower[i]) {
            for (int j = 0; j < i; j++) free(tower[j]);
            free(tower);
            return NULL;
        }

        int stars = 2 * i + 1;
        int pad = (width - stars) / 2;

        memset(tower[i], ' ', pad);
        memset(tower[i] + pad, '*', stars);
        memset(tower[i] + pad + stars, ' ', pad);
        tower[i][width] = '\0';
    }

    return tower;
}

void free_tower(char **tower, int nFloors) {
    if (!tower) return;
    for (int i = 0; i < nFloors; i++) free(tower[i]);
    free(tower);
}

int main(void) {
    int n = 6;
    char **tower = build_tower(n);
    if (!tower) return 1;

    printf("[\n");
    for (int i = 0; i < n; i++) {
        printf("  \"%s\"%s\n", tower[i], (i < n - 1) ? "," : "");
    }
    printf("]\n");

    free_tower(tower, n);
    return 0;
}