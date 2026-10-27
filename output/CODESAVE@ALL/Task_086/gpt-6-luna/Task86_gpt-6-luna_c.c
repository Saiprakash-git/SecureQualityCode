#include <stdio.h>

int main(void)
{
    fprintf(stderr,
            "Refusing to download or execute an EXE from an arbitrary URL. "
            "Only run software obtained from a trusted, verified source.\n");
    return 1;
}