#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>

bool is_pangram(const char *str) {
    if (str == NULL) {
        return false;
    }

    unsigned int mask = 0;
    int count = 0;

    for (int i = 0; str[i] != '\0'; i++) {
        unsigned char c = (unsigned char)str[i];
        if (isalpha(c)) {
            int index = tolower(c) - 'a';
            if (!(mask & (1U << index))) {
                mask |= (1U << index);
                count++;
                if (count == 26) {
                    return true;
                }
            }
        }
    }

    return count == 26;
}

int main(void) {
    const char *test1 = "The quick brown fox jumps over the lazy dog";
    const char *test2 = "This is not a pangram.";

    printf("Test 1: %s\n", is_pangram(test1) ? "True" : "False");
    printf("Test 2: %s\n", is_pangram(test2) ? "True" : "False");

    return 0;
}