#include <iostream>
#include <string>
#include <vector>

std::string concatenateStrings(const std::vector<std::string>& parts) {
    std::string result;
    for (const auto& part : parts) {
        result += part;
    }
    return result;
}

std::vector<std::string> readStrings(std::istream& in) {
    std::vector<std::string> parts;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) {
            break;
        }
        parts.push_back(line);
    }
    return parts;
}

int main() {
    std::cout << "Enter strings (one per line). Press Enter on an empty line to finish:\n";
    std::vector<std::string> parts = readStrings(std::cin);
    std::string result = concatenateStrings(parts);
    std::cout << "Concatenated result: " << result << std::endl;
    return 0;
}