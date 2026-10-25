#include <stdbool.h>
#include <string.h>
#include <ctype.h>

bool validate_pin(const char *pin) {
    if (pin == NULL) {
        return false;
    }

    size_t len = strlen(pin);

    if (len != 4 && len != 6) {
        return false;
    }

    for (size_t i = 0; i < len; i++) {
        if (!isdigit((unsigned char)pin[i])) {
            return false;
        }
    }

    return true;
}