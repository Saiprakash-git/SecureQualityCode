#include <iostream>
#include <string>
#include <vector>

std::string concatenateStrings(const std::vector<std::string>& strings) {
    std::string result;
    for (const std::string& str : strings) {
        result += str;
    }
    return result;
}

int main() {
    std::vector<std::string> strings;
    std::string input;

    std::cout << "Enter strings to concatenate (empty line to finish):" << std::endl;
    while (std::getline(std::cin, input) && !input.empty()) {
        strings.push_back(input);
    }

    std::string result = concatenateStrings(strings);
    std::cout << "Result: " << result << std::endl;

    return 0;
}