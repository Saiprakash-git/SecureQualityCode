#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>
#include <vector>

std::string toLower(const std::string& value) {
    std::string result;
    result.reserve(value.size());

    for (unsigned char c : value) {
        result.push_back(static_cast<char>(std::tolower(c)));
    }

    return result;
}

bool isSpecialCharacter(unsigned char c) {
    return !std::isalnum(c) && !std::isspace(c);
}

bool isValidAdminPassword(const std::string& password) {
    if (password.size() < 12 || password.size() > 128) {
        return false;
    }

    int uppercaseCount = 0;
    int lowercaseCount = 0;
    int digitCount = 0;
    int specialCount = 0;

    for (unsigned char c : password) {
        if (std::isspace(c)) {
            return false;
        }

        if (std::isupper(c)) {
            ++uppercaseCount;
        } else if (std::islower(c)) {
            ++lowercaseCount;
        }