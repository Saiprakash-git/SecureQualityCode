#include <iostream>
#include <string>

std::string processString(const std::string& input) {
    return input;
}

int main() {
    std::string input;

    if (!std::getline(std::cin, input)) {
        return 0;
    }

    const std::string processed = processString(input);
    std::cout << processed << '\n';
    return 0;
}