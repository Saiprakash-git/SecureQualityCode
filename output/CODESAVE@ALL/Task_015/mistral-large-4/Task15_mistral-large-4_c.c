#include <stdlib.h>
#include <string.h>

char** build_tower(int floors) {
    if (floors <= 0) return NULL;
    int width = 2 * floors - 1;
    char** tower = (char**)malloc(floors * sizeof(char*));
    for (int i = 0; i < floors; i++) {
        tower[i] = (char*)malloc(width + 1);
        int stars = 2 * i + 1;
        int spaces = (width - stars) / 2;
        memset(tower[i], ' ', width);
        memset(tower[i] + spaces, '*', stars);
        tower[i][width] = '\0';
    }
    return tower;
}

void free_tower(char** tower, int floors) {
    if (!tower) return;
    for (int i = 0; i < floors; i++) {
        free(tower[i]);
    }
    free(tower);
}