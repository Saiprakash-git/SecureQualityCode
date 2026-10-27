#include <string>

bool validate_pin(const std::string& pin) {
    if (pin.size() != 4 && pin.size() != 6) {
        return false;
    }

    for (char digit : pin) {
        if (digit < '0' || digit > '9') {
            return false;
        }
    }

    return true;
}