#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <functional>
#include <iomanip>

struct User {
    std::string name;
    std::string email;
    std::string passwordHash;
};

class UserDatabase {
private:
    std::string filename;

    std::string hashPassword(const std::string& password) {
        std::hash<std::string> hasher;
        size_t hash1 = hasher(password + "salt_registration_2024");
        size_t hash2 = hasher(std::to_string(hash1) + password);
        std::stringstream ss;
        ss << std::hex << hash1 << hash2;
        return ss.str();
    }

    bool isValidEmail(const std::string& email) {
        if (email.empty()) return false;
        auto atPos = email.find('@');
        if (atPos == std::string::npos || atPos == 0 || atPos == email.length() - 1) {
            return false;
        }
        auto dotPos = email.find('.', atPos);
        if (dotPos == std::string::npos || dotPos == atPos + 1 || dotPos == email.length() - 1) {
            return false;
        }
        for (char c : email) {
            if (!isalnum(c) && c != '@' && c != '.' && c != '_' && c != '-') {
                return false;
            }
        }
        return true;
    }

    std::string escapeField(const std::string& field) {
        std::string result;
        for (char c : field) {
            if (c == '|' || c == '\n') {
                result += '_';
            } else {
                result += c;
            }
        }
        return result;
    }

public:
    UserDatabase(const std::string& dbFile) : filename(dbFile) {
        std::ofstream file(filename, std::ios::app);
        if (!file.is_open()) {
            std::cerr << "Warning: Could not create database file." << std::endl;
        }
    }

    bool userExists(const std::string& email) {
        std::ifstream file(filename);
        if (!file.is_open()) return false;

        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string name, storedEmail, hash;
            if (std::getline(iss, name, '|') &&
                std::getline(iss, storedEmail, '|') &&
                std::getline(iss, hash)) {
                if (storedEmail == email) {
                    return true;
                }
            }
        }
        return false;
    }

    bool registerUser(const std::string& name, const std::string& email, const std::string& password) {
        if (name.empty()) {
            std::cout << "Error: Name cannot be empty." << std::endl;
            return false;
        }

        if (email.empty()) {
            std::cout << "Error: Email cannot be empty." << std::endl;
            return false;
        }

        if (!isValidEmail(email)) {
            std::cout << "Error: Invalid email format." << std::endl;
            return false;
        }

        if (password.length() < 6) {
            std::cout << "Error: Password must be at least 6 characters long." << std::endl;
            return false;
        }

        if (userExists(email)) {
            std::cout << "Error: A user with this email already exists." << std::endl;
            return false;
        }

        std::string hashedPassword = hashPassword(password);
        std::string safeName = escapeField(name);
        std::string safeEmail = escapeField(email);

        std::ofstream file(filename, std::ios::app);
        if (!file.is_open()) {
            std::cout << "Error: Could not write to database." << std::endl;
            return false;
        }

        file << safeName << "|" << safeEmail << "|" << hashedPassword << std::endl;
        file.close();

        std::cout << "Success: User registered successfully." << std::endl;
        return true;
    }

    std::vector<User> getAllUsers() {
        std::vector<User> users;
        std::ifstream file(filename);
        if (!file.is_open()) return users;

        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            User user;
            if (std::getline(iss, user.name, '|') &&
                std::getline(iss, user.email, '|') &&
                std::getline(iss, user.passwordHash)) {
                users.push_back(user);
            }
        }
        return users;
    }

    void displayUsers() {
        std::vector<User> users = getAllUsers();
        if (users.empty()) {
            std::cout << "No users registered." << std::endl;
            return;
        }

        std::cout << "\n--- Registered Users ---" << std::endl;
        std::cout << std::left << std::setw(20) << "Name" << std::setw(30) << "Email" << std::endl;
        std::cout << std::string(50, '-') << std::endl;
        for (const auto& user : users) {
            std::cout << std::left << std::setw(20) << user.name
                      << std::setw(30) << user.email << std::endl;
        }
    }
};

std::string getInput(const std::string& prompt) {
    std::string input;
    std::cout << prompt;
    std::getline(std::cin, input);
    return input;
}

void registrationForm(UserDatabase& db) {
    std::cout << "\n=== New User Registration ===" << std::endl;

    std::string name = getInput("Enter full name: ");
    std::string email = getInput("Enter email address: ");
    std::string password = getInput("Enter password (min 6 chars): ");

    db.registerUser(name, email, password);
}

int main() {
    UserDatabase database("users.db");

    int choice = 0;
    while (choice != 3) {
        std::cout << "\n=== User Registration System ===" << std::endl;
        std::cout << "1. Register new user" << std::endl;
        std::cout << "2. View all users" << std::endl;
        std::cout << "3. Exit" << std::endl;
        std::cout << "Enter choice: ";

        std::string input;
        std::getline(std::cin, input);

        try {
            choice = std::stoi(input);
        } catch (...) {
            std::cout << "Invalid input. Please enter a number." << std::endl;
            continue;
        }

        switch (choice) {
            case 1:
                registrationForm(database);
                break;
            case 2:
                database.displayUsers();
                break;
            case 3:
                std::cout << "Exiting system. Goodbye!" << std::endl;
                break;
            default:
                std::cout << "Invalid choice. Please try again." << std::endl;
                break;
        }
    }

    return 0;
}