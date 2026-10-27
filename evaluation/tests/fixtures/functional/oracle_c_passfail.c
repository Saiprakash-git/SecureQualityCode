#include <stdio.h>
#include "../../source/output/claude/c/Task900_CLAUDE_x.c"

static void run_test(int got, int expected, const char *name) {
    if (got == expected) printf("%s: PASS\n", name); else printf("%s: FAIL\n", name);
}

int main(void) {
    run_test(add(2, 3), 5, "small");
    run_test(add(-4, 1), -3, "negative");
    return 0;
}
