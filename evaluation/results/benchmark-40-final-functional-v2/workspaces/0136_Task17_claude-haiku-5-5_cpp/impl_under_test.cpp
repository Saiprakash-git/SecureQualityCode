#include <iostream>
#include <string>
#include <cctype>

std::string toJadenCase(const std::string& phrase) {
    if (phrase.empty()) {
        return "";
    }
    std::string result = phrase;
    bool startOfWord = true;
    for (size_t i = 0; i < result.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(result[i]);
        if (std::isspace(c)) {
            startOfWord = true;
        } else {
            result[i] = startOfWord
                ? static_cast<char>(std::toupper(c))
                : static_cast<char>(std::tolower(c));
            startOfWord = false;
        }
    }
    return result;
}

int main() {
    std::string line;
    if (std::getline(std::cin, line)) {
        std::cout << toJadenCase(line) << std::endl;
    }
    return 0;
}