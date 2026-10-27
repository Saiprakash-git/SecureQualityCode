#include <iostream>
#include <fstream>
#include <string>

static void removeTrailingCarriageReturn(std::string& value) {
    if (!value.empty() && value.back() == '\r') {
        value.pop_back();
    }
}

static bool readStoredPassword(const std::string& filename, std::string& storedPassword) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    std::getline(file, storedPassword);
    removeTrailingCarriageReturn(storedPassword);
    return true;
}

static bool readEnteredPassword(std::istream& input, std::string& enteredPassword) {
    std::cout << "Enter password: ";
    if (!std::getline(input, enteredPassword)) {
        return false;
    }

    removeTrailingCarriageReturn(enteredPassword);
    return true;
}

static bool comparePasswordsSequentially(const std::string& storedPassword, const std::string& enteredPassword) {
    if (storedPassword.length() != enteredPassword.length()) {
        return false;
    }

    for (std::size_t index = 0; index < storedPassword.length(); ++index) {
        if (storedPassword[index] != enteredPassword[index]) {
            return false;
        }
    }

    return true;
}

int main(int argc, char* argv[]) {
    std::string filename = "password.txt";
    if (argc > 1) {
        filename = argv[1];
    }

    std::string storedPassword;
    std::string enteredPassword;

    if (!readStoredPassword(filename, storedPassword)) {
        std::cerr << "Error: Could not open stored password file: " << filename << '\n';
        return 1;
    }

    if (!readEnteredPassword(std::cin, enteredPassword)) {
        std::cerr << "Error: Could not read entered password.\n";
        return 1;
    }

    if (comparePasswordsSequentially(storedPassword, enteredPassword)) {
        std::cout << "Passwords match.\n";
    } else {
        std::cout << "Passwords do not match.\n";
    }

    return 0;
}