#include <iostream>
#include <string>
#include <cctype>

bool validatePIN(const std::string& pin) {
    if (pin.length() != 4 && pin.length() != 6) {
        return false;
    }
    for (char c : pin) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            return false;
        }
    }
    return true;
}

int main() {
    std::cout << std::boolalpha;
    std::cout << validatePIN("1234") << std::endl;
    std::cout << validatePIN("12345") << std::endl;
    std::cout << validatePIN("a234") << std::endl;
    return 0;
}