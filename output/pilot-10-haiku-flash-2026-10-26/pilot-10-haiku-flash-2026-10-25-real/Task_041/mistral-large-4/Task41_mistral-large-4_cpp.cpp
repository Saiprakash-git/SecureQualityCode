#include <iostream>
#include <string>
#include <cctype>

std::string processString(const std::string& input) {
    std::string result;
    result.reserve(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(input[i]);
        if (std::isalpha(c)) {
            result.push_back(static_cast<char>(std::toupper(c)));
        } else {
            result.push_back(static_cast<char>(c));
        }
    }
    return result;
}

int main() {
    std::string userInput;
    std::cout << "Enter a string: ";
    std::getline(std::cin, userInput);

    std::string processed = processString(userInput);
    std::cout << "Processed: " << processed << std::endl;
    std::cout << "Length: " << processed.length() << std::endl;

    return 0;
}