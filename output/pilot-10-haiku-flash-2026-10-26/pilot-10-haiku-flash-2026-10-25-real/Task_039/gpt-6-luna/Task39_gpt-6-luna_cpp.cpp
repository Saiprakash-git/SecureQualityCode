#include <cstdlib>
#include <iostream>
#include <string>

std::string shellQuote(const std::string& value) {
    std::string quoted = "'";
    for (char ch : value) {
        if (ch == '\'') {
            quoted += "'\\''";
        } else {
            quoted += ch;
        }
    }
    quoted += "'";
    return quoted;
}

int main() {
    std::string filename;
    std::cout << "Enter a filename: ";
    if (!std::getline(std::cin, filename)) {
        return 1;
    }

    const std::string command = "cat -- " + shellQuote(filename);
    return std::system(command.c_str());
}