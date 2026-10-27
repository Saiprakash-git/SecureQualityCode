#include <fstream>
#include <iostream>
#include <string>

bool passwordsMatch(const std::string& storedPassword,
                    const std::string& enteredPassword) {
    if (storedPassword.length() != enteredPassword.length()) {
        return false;
    }

    for (std::size_t i = 0; i < storedPassword.length(); ++i) {
        if (storedPassword[i] != enteredPassword[i]) {
            return false;
        }
    }

    return true;
}

int main(int argc, char* argv[]) {
    const std::string fileName = argc > 1 ? argv[1] : "password.txt";
    std::ifstream passwordFile(fileName);

    if (!passwordFile) {
        std::cerr << "Could not open password file: " << fileName << '\n';
        return 1;
    }

    std::string storedPassword;
    if (!std::getline(passwordFile, storedPassword)) {
        std::cerr << "Could not read password from file.\n";
        return 1;
    }

    std::string enteredPassword;
    std::cout << "Enter password: ";
    if (!std::getline(std::cin, enteredPassword)) {
        std::cerr << "Could not read entered password.\n";
        return 1;
    }

    if (passwordsMatch(storedPassword, enteredPassword)) {
        std::cout << "Password matches.\n";
    } else {
        std::cout << "Password does not match.\n";
    }

    return 0;
}