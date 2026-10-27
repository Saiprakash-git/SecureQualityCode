#include <iostream>
#include <string>
#include <cctype>
#include <algorithm>

struct ValidationResult {
    bool isValid;
    std::string errorMessage;
};

ValidationResult validateAdminPassword(const std::string& password) {
    const size_t MIN_LENGTH = 12;

    if (password.length() < MIN_LENGTH) {
        return {false, "Password must be at least 12 characters long."};
    }

    bool hasUpper = false;
    bool hasLower = false;
    bool hasDigit = false;
    bool hasSpecial = false;

    for (char ch : password) {
        if (std::isspace(static_cast<unsigned char>(ch))) {
            return {false, "Password must not contain whitespace characters."};
        }
        if (std::isupper(static_cast<unsigned char>(ch))) {
            hasUpper = true;
        } else if (std::islower(static_cast<unsigned char>(ch))) {
            hasLower = true;
        } else if (std::isdigit(static_cast<unsigned char>(ch))) {
            hasDigit = true;
        } else if (std::ispunct(static_cast<unsigned char>(ch))) {
            hasSpecial = true;
        }
    }

    if (!hasUpper) {
        return {false, "Password must contain at least one uppercase letter."};
    }
    if (!hasLower) {
        return {false, "Password must contain at least one lowercase letter."};
    }
    if (!hasDigit) {
        return {false, "Password must contain at least one numerical digit."};
    }
    if (!hasSpecial) {
        return {false, "Password must contain at least one special character."};
    }

    std::string lowerPassword = password;
    std::transform(lowerPassword.begin(), lowerPassword.end(), lowerPassword.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    if (lowerPassword.find("admin") != std::string::npos) {
        return {false, "Password must not contain the word 'admin'."};
    }
    if (lowerPassword.find("password") != std::string::npos) {
        return {false, "Password must not contain the word 'password'."};
    }

    return {true, "Password is valid for an admin user."};
}

int main() {
    std::cout << "Enter admin password to validate: ";
    std::string password;
    if (std::getline(std::cin, password)) {
        ValidationResult result = validateAdminPassword(password);
        if (result.isValid) {
            std::cout << "SUCCESS: " << result.errorMessage << std::endl;
        } else {
            std::cout << "FAILURE: " << result.errorMessage << std::endl;
        }
    }
    return 0;
}