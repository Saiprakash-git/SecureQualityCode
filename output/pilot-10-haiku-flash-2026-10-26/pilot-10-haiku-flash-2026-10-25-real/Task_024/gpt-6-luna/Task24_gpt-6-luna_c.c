#include <stdbool.h>
#include <stddef.h>

bool validate_pin(const char *pin)
{
    if (pin == NULL)
        return false;

    size_t length = 0;
    while (pin[length] != '\0') {
        if (pin[length] < '0' || pin[length] > '9')
            return false;
        ++length;
    }

    return length == 4 || length == 6;
}