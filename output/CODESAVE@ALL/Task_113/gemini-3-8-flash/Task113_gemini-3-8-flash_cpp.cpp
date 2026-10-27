#include <iostream>
#include <vector>
#include <string>

std::string concatenateStrings(const std::vector<std::string>& strings) {
    std::string result;
    for (const auto& str : strings) {
        result += str;
    }
    return result;
}

int main() {
    std::cout << "Enter the number of strings to concatenate: ";
    int count = 0;
    if (!(std::cin >> count) || count <= 0) {
        std::cout << "Invalid count entered." << std::endl;
        return 0;
    }

    std::cin.ignore();

    std::vector<std::string> inputs;
    inputs.reserve(count);

    for (int i = 0; i < count; ++i) {
        std::cout << "Enter string " << (i + 1) << ": ";
        std::string current;
        std::getline(std::cin, current);
        inputs.push_back(current);
    }

    std::string result = concatenateStrings(inputs);

    std::cout << "\nConcatenated string:\n" << result << std::endl;

    return 0;
}