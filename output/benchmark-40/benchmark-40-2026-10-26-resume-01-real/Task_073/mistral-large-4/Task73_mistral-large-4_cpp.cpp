#include <iostream>
#include <string>
#include <cctype>

bool isStrongPassword(const std::string& password) {
    if (password.length() < 8) {
        return false;
    }

    bool hasUpper = false;
    bool hasLower = false;
    bool hasDigit = false;
    bool hasSpecial = false;

    for (char c : password) {
        if (isupper(static_cast<unsigned char>(c))) {
            hasUpper = true;
        } else if (islower(static_cast<unsigned char>(c))) {
            hasLower = true;
        } else if (isdigit(static_cast<unsigned char>(c))) {
            hasDigit = true;
        } else {
            hasSpecial = true;
        }
    }

    return hasUpper && hasLower && hasDigit && hasSpecial;
}

bool checkAdminPassword(const std::string& inputPassword, const std::string& storedPassword) {
    return inputPassword == storedPassword;
}

bool isValidAdminPassword(const std::string& password, const std::string& storedAdminPassword) {
    if (!isStrongPassword(password)) {
        return false;
    }
    return checkAdminPassword(password, storedAdminPassword);
}

int main() {
    std::string storedAdminPassword = "Admin@123";
    std::string inputPassword;

    std::cout << "Enter admin password: ";
    std::getline(std::cin, inputPassword);

    if (isValidAdminPassword(inputPassword, storedAdminPassword)) {
        std::cout << "Password is valid for admin user." << std::endl;
        return 0;
    } else {
        std::cout << "Password is invalid." << std::endl;
        return 1;
    }
}