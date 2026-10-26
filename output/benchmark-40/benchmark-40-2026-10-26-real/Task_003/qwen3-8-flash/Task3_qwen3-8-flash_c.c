#include <stdbool.h>

bool is_pangram(const char *str) {
    unsigned int mask = 0;

    if (!str) {
        return false;
    }

    for (; *str != '\0'; ++str) {
        if (*str >= 'a' && *str <= 'z') {
            mask |= 1u << (*str - 'a');
        } else if (*str >= 'A' && *str <= 'Z') {
            mask |= 1u << (*str - 'A');
        }
    }

    return mask == ((1u << 26) - 1u);
}