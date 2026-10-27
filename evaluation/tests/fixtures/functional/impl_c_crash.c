#include <stdlib.h>
int add(int a, int b) { volatile int *p = NULL; (void)a; (void)b; exit(7); return *p; }
