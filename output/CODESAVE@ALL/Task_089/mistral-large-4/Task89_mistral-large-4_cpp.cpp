#include <iostream>
#include <fstream>
#include <string>

std::string readPasswordFromFile(const std::string& filename) {
    std::ifstream file(filename);
    std::string password;
    if (file.is_open()) {
        std::getline(file, password);
        file.close();
    } else {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return "";
    }
    return password;
}

bool comparePasswordsSequentially(const std::string& stored, const std::string& input) {
    if (stored.length() != input.length()) {
        return false;
    }
    for (size_t i = 0; i < stored.length(); i++) {
        if (stored[i] != input[i]) {
            return false;
        }
    }
    return true;
}

int main() {
    std::string filename = "password.txt";
    std::string storedPassword = readPasswordFromFile(filename);

    if (storedPassword.empty()) {
        std::cerr << "Failed to read password from file." << std::endl;
        return 1;
    }

    std::string userInput;
    std::cout << "Enter password: ";
    std::getline(std::cin, userInput);

    bool isMatch = comparePasswordsSequentially(storedPassword, userInput);

    if (isMatch) {
        std::cout << "Password matches!" << std::endl;
        return 0;
    } else {
        std::cout << "Password does not match." << std::endl;
        return 1;
    }
}