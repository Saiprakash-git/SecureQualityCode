#include <string>

bool validatePIN(const std::string& pin) {
    if (pin.size() != 4 && pin.size() != 6) {
        return false;
    }

    for (char c : pin) {
        if (c < '0' || c > '9') {
            return false;
        }
    }

    return true;
}