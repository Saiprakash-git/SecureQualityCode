#include <iostream>
#include <string>
#include <cctype>

bool isValidAdminPassword(const std::string& password) {
    if (password.size() < 8) {
        return false;
    }

    bool hasUppercase = false;
    bool hasLowercase = false;
    bool hasDigit = false;
    bool hasSpecial = false;

    for (unsigned char ch : password) {
        if (std::isupper(ch)) {
            hasUppercase = true;
        } else if (std::islower(ch)) {
            hasLowercase = true;
        } else if (std::isdigit(ch)) {
            hasDigit = true;
        } else if (!std::isspace(ch)) {
            hasSpecial = true;
        } else {
            return false;
        }
    }

    return hasUppercase && hasLowercase && hasDigit && hasSpecial;
}

int main() {
    std::string password;
    std::getline(std::cin, password);

    std::cout << (isValidAdminPassword(password) ? "Valid" : "Invalid") << '\n';
    return 0;
}