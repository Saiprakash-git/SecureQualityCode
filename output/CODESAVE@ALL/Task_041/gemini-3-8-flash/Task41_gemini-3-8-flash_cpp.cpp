#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>

std::string processString(const std::string& input) {
    std::string result = input;
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    std::reverse(result.begin(), result.end());
    return result;
}

int main() {
    std::cout << "Enter a string to process: ";
    
    std::string userInput;
    if (std::getline(std::cin, userInput)) {
        std::string processedOutput = processString(userInput);
        std::cout << "Processed string: " << processedOutput << '\n';
    } else {
        std::cerr << "Error reading input.\n";
        return 1;
    }

    return 0;
}