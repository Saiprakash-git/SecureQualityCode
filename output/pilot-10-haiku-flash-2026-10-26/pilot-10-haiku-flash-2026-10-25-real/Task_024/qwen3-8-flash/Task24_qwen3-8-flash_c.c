#include <stdbool.h>

bool validate_pin(const char *pin) {
    if (!pin) {
        return false;
    }

    int length = 0;

    while (*pin) {
        if (*pin < '0' || *pin > '9') {
            return false;
        }

        length++;
        pin++;
    }

    return length == 4 || length == 6;
}