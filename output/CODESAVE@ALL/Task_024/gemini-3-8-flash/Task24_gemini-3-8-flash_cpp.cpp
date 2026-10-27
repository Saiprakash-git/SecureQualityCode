#include <string>

bool validate_pin(const std::string& pin) {
    if (pin.length() != 4 && pin.length() != 6) {
        return false;
    }
    for (char c : pin) {
        if (c < '0' || c > '9') {
            return false;
        }
    }
    return true;
}

int main() {
    return 0;
}