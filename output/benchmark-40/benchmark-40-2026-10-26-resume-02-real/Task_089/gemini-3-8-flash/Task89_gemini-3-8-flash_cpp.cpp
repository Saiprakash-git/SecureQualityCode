#include <iostream>
#include <fstream>
#include <string>

bool writePasswordToFile(const std::string& filepath, const std::string& password) {
    std::ofstream outFile(filepath);
    if (!outFile.is_open()) {
        return false;
    }
    outFile << password;
    outFile.close();
    return true;
}

std::string readPasswordFromFile(const std::string& filepath, bool& success) {
    std::ifstream inFile(filepath);
    if (!inFile.is_open()) {
        success = false;
        return "";
    }
    std::string password;
    std::getline(inFile, password);
    inFile.close();
    success = true;
    return password;
}

bool comparePasswordsSequentially(const std::string& storedPassword, const std::string& enteredPassword) {
    if (storedPassword.length() != enteredPassword.length()) {
        return false;
    }
    for (size_t i = 0; i < storedPassword.length(); ++i) {
        if (storedPassword[i] != enteredPassword[i]) {
            return false;
        }
    }
    return true;
}

int main() {
    const std::string filepath = "password.txt";
    
    // Setup a default password file if it does not already exist
    bool readSuccess = false;
    std::string storedPassword = readPasswordFromFile(filepath, readSuccess);
    if (!readSuccess) {
        writePasswordToFile(filepath, "Secret123");
        storedPassword = readPasswordFromFile(filepath, readSuccess);
        if (!readSuccess) {
            std::cerr << "Error: Unable to initialize or open password file." << std::endl;
            return 1;
        }
    }

    std::cout << "Enter your password: ";
    std::string enteredPassword;
    std::getline(std::cin, enteredPassword);

    bool isMatch = comparePasswordsSequentially(storedPassword, enteredPassword);

    if (isMatch) {
        std::cout << "Access granted: Passwords match sequentially." << std::endl;
    } else {
        std::cout << "Access denied: Passwords do not match." << std::endl;
    }

    return 0;
}