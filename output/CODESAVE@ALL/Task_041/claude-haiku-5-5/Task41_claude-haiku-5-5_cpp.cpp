#include <iostream>
#include <string>
#include <limits>
#include <algorithm>
#include <cctype>

std::string toUpperCase(const std::string& input) {
    std::string result = input;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return result;
}

std::string reverseString(const std::string& input) {
    return std::string(input.rbegin(), input.rend());
}

std::string removeWhitespace(const std::string& input) {
    std::string result;
    result.reserve(input.size());
    for (unsigned char c : input) {
        if (!std::isspace(c)) {
            result.push_back(static_cast<char>(c));
        }
    }
    return result;
}

size_t countVowels(const std::string& input) {
    size_t count = 0;
    for (unsigned char c : input) {
        char lc = static_cast<char>(std::tolower(c));
        if (lc == 'a' || lc == 'e' || lc == 'i' || lc == 'o' || lc == 'u') {
            ++count;
        }
    }
    return count;
}

std::string processInput(const std::string& input) {
    std::string output;
    output += "Original length: " + std::to_string(input.size()) + "\n";
    output += "Uppercase: " + toUpperCase(input) + "\n";
    output += "Reversed: " + reverseString(input) + "\n";
    output += "Without whitespace: " + removeWhitespace(input) + "\n";
    output += "Vowel count: " + std::to_string(countVowels(input)) + "\n";
    return output;
}

std::string readLineSafely(std::istream& in, size_t maxLength) {
    std::string line;
    line.reserve(std::min<size_t>(maxLength, 256));
    char ch;
    while (in.get(ch)) {
        if (ch == '\n') {
            break;
        }
        if (line.size() < maxLength) {
            line.push_back(ch);
        }
    }
    if (line.size() >= maxLength) {
        std::cerr << "Warning: input truncated to " << maxLength << " characters.\n";
    }
    return line;
}

int main() {
    const size_t MAX_INPUT_LENGTH = 1024;

    std::cout << "Enter a string (max " << MAX_INPUT_LENGTH << " characters):\n";
    std::string input = readLineSafely(std::cin, MAX_INPUT_LENGTH);

    if (!std::cin && !std::cin.eof()) {
        std::cerr << "Error reading input.\n";
        return 1;
    }

    std::cout << processInput(input);
    return 0;
}