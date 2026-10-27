#include <iostream>
#include <fstream>
#include <string>
#include <cstddef>

std::string readPasswordFromFile(const std::string& path, bool& success) {
    std::ifstream file(path);
    success = false;
    if (!file) {
        return "";
    }

    std::string password;
    std::getline(file, password);

    while (!password.empty() && (password.back() == '\r' || password.back() == '\n')) {
        password.pop_back();
    }

    success = true;
    return password;
}

bool comparePasswords(const std::string& stored, const std::string& entered) {
    if (stored.length() != entered.length()) {
        return false;
    }

    for (std::size_t i = 0; i < stored.length(); ++i) {
        if (stored[i] != entered[i]) {
            return false;
        }
    }

    return true;
}

std::string getUserInput(const std::string& prompt) {
    std::cout << prompt;
    std::string input;
    std::getline(std::cin, input);
    return input;
}

int main(int argc, char* argv[]) {
    std::string filePath = (argc > 1) ? argv[1] : "password.txt";

    bool fileOk = false;
    std::string storedPassword = readPasswordFromFile(filePath, fileOk);

    if (!fileOk) {
        std::cerr << "Error: could not open file '" << filePath << "'." << std::endl;
        return 1;
    }

    std::string enteredPassword = getUserInput("Enter your password: ");

    if (comparePasswords(storedPassword, enteredPassword)) {
        std::cout << "Password matches. Access granted." << std::endl;
        return 0;
    }

    std::cout << "Password does not match. Access denied." << std::endl;
    return 2;
}