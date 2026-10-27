#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

struct PasswordResult {
    bool valid;
    std::vector<std::string> errors;
};

std::string toLower(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

bool hasRepeatedRun(const std::string& pwd, size_t maxRun) {
    size_t run = 1;
    for (size_t i = 1; i < pwd.size(); ++i) {
        if (pwd[i] == pwd[i - 1]) {
            ++run;
            if (run > maxRun) return true;
        } else {
            run = 1;
        }
    }
    return false;
}

PasswordResult validateAdminPassword(const std::string& password,
                                     const std::string& username) {
    PasswordResult result{true, {}};
    const size_t minLength = 12;
    const size_t maxLength = 128;

    if (password.size() < minLength) {
        result.errors.push_back("Password must be at least " + std::to_string(minLength) + " characters long.");
    }
    if (password.size() > maxLength) {
        result.errors.push_back("Password must not exceed " + std::to_string(maxLength) + " characters.");
    }

    bool hasUpper = false, hasLower = false, hasDigit = false, hasSpecial = false, hasSpace = false;
    for (unsigned char c : password) {
        if (std::isupper(c)) hasUpper = true;
        else if (std::islower(c)) hasLower = true;
        else if (std::isdigit(c)) hasDigit = true;
        else if (std::isspace(c)) hasSpace = true;
        else if (std::isprint(c)) hasSpecial = true;
        else {
            hasSpecial = true;
        }
    }

    if (!hasUpper) result.errors.push_back("Password must contain at least one uppercase letter.");
    if (!hasLower) result.errors.push_back("Password must contain at least one lowercase letter.");
    if (!hasDigit) result.errors.push_back("Password must contain at least one digit.");
    if (!hasSpecial) result.errors.push_back("Password must contain at least one special character.");
    if (hasSpace) result.errors.push_back("Password must not contain whitespace.");

    if (hasRepeatedRun(password, 2)) {
        result.errors.push_back("Password must not contain three or more identical characters in a row.");
    }

    std::string lowerPwd = toLower(password);
    std::string lowerUser = toLower(username);

    if (!lowerUser.empty() && lowerPwd.find(lowerUser) != std::string::npos) {
        result.errors.push_back("Password must not contain the username.");
    }
    if (lowerPwd.find("admin") != std::string::npos) {
        result.errors.push_back("Password must not contain the word \"admin\".");
    }
    if (lowerPwd.find("password") != std::string::npos) {
        result.errors.push_back("Password must not contain the word \"password\".");
    }

    result.valid = result.errors.empty();
    return result;
}

int main() {
    std::string username;
    std::string password;

    std::cout << "Enter admin username: ";
    std::getline(std::cin, username);

    std::cout << "Enter admin password: ";
    std::getline(std::cin, password);

    PasswordResult result = validateAdminPassword(password, username);

    if (result.valid) {
        std::cout << "Password is VALID for admin user.\n";
        return 0;
    }

    std::cout << "Password is INVALID for admin user:\n";
    for (const auto& err : result.errors) {
        std::cout << " - " << err << "\n";
    }
    return 1;
}