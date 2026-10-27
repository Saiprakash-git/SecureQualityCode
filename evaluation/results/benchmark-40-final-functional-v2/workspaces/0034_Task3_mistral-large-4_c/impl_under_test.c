#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>

bool is_pangram(const char *str) {
    int mask = 0;
    const int target = (1 << 26) - 1;
    for (int i = 0; str[i] != '\0' && mask != target; i++) {
        unsigned char c = (unsigned char)str[i];
        if (isalpha(c)) {
            mask |= 1 << (tolower(c) - 'a');
        }
    }
    return mask == target;
}

int main(void) {
    char input[10000];
    if (fgets(input, sizeof(input), stdin) != NULL) {
        printf("%s\n", is_pangram(input) ? "True" : "False");
    }
    return 0;
}