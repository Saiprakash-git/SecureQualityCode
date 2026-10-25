#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <limits>
#include <cctype>
#include <cstdint>
#include <sstream>
#include <iomanip>

struct User {
    std::string name;
    std::string email;
    std::string passwordHash;
};

class UserDatabase {
public:
    bool registerUser(const std::string& name,
                      const std::string& email,
                      const std::string& password,
                      std::string& errorMessage);

    bool emailExists(const std::string& email) const;
    std::vector<User> getAllUsers() const;
    std::size_t userCount() const;

private:
    std::unordered_map<std::string, User> users;

    static std::string trim(const std::string& value);
    static std::string normalizeEmail(const std::string& email);
    static bool isValidEmail(const std::string& email);
    static bool isValidPassword(const std::string& password);
    static std::string hashPassword(const std::string& password);
};

bool UserDatabase::registerUser(const std::string& name,
                                 const std::string& email,
                                 const std::string& password,
                                 std::string& errorMessage) {
    std::string cleanName = trim(name);
    std::string cleanEmail = normalizeEmail(trim(email));

    if (cleanName.empty()) {
        errorMessage = "Name cannot be empty.";
        return false;
    }

    if (!isValidEmail(cleanEmail)) {
        errorMessage = "Invalid email address.";
        return false;
    }

    if (!isValidPassword(password)) {
        errorMessage = "Password must be at least 8 characters and contain at least one letter and one digit.";
        return false;
    }

    if (users.find(cleanEmail) != users.end()) {
        errorMessage = "Email already registered.";
        return false;
    }

    users[cleanEmail] = User{cleanName, cleanEmail, hashPassword(password)};
    return true;
}

bool UserDatabase::emailExists(const std::string& email) const {
    return users.find(normalizeEmail(trim(email))) != users.end();
}

std::vector<User> UserDatabase::getAllUsers() const {
    std::vector<User> result;
    result.reserve(users.size());

    for (const auto& pair : users) {
        result.push_back(pair.second);
    }

    return result;
}

std::size_t UserDatabase::userCount() const {
    return users.size();
}

std::string UserDatabase::trim(const std::string& value) {
    if (value.empty()) {
        return value;
    }

    std::size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) {
        ++start;
    }

    std::size_t end = value