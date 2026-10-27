#include <stdbool.h>
#include <stddef.h>

bool validate_pin(const char *pin) {
    if (!pin) {
        return false;
    }

    size_t len = 0;
    while (pin[len] != '\0') {
        if (pin[len] < '0' || pin[len] > '9') {
            return false;
        }
        len++;
    }

    return (len == 4 || len == 6);
}