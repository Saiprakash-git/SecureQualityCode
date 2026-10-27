#include <assert.h>
#include <stdio.h>
#include "../../source/output/claude/c/Task900_CLAUDE_x.c"  // Include the function
// #include "../../source/output/gemini/c/Task900_GEMINI_x.c"

int main(void) {
    assert(add(2, 3) == 5);
    assert(add(-4, 1) == -3);
    printf("all assertions passed\n");
    return 0;
}
