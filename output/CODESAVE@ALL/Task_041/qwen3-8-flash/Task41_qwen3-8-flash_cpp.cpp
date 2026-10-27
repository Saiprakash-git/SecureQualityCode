#include <iostream>
#include <string>
#include <cctype>
#include <algorithm>

std::string trim(const std::string& input) {
    const std::string whitespace = " \t\n\r\f\v";

    std::size_t start = input.find_first_not_of(whitespace);
    if (start == std::string::npos) {
        return "";
    }

    std::size_t end = input.find_last_not_of(whitespace);
    return input.substr(start, end - start + 1);
}

std::string toUpper(const std::string& input) {
    std::string result = input;

    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) {
            return static_cast<char>(std::toupper(c));
        });

    return result;
}

int countWords(const std::string& input) {
    if (input.empty()) {
        return 0;
    }

    int count = 0;
    bool inWord = false;

    for (unsigned char c : input) {
        if (std::isspace(c)) {
            inWord = false;
        } else if (!inWord) {
            ++count;
            inWord = true;
        }
    }

    return count;
}

std::string processInput(const std::string& input) {
    std::string trimmed = trim(input);
    std::string upper = toUpper(trimmed);
    int words = countWords(trimmed);

    return upper + " [" + std::to_string(words) + " word(s)]";
}

int main() {
    std::string input;

    std::cout << "Enter a string: ";
    std::getline(std::cin, input);

    std::cout << processInput(input) << std::endl;

    return 0;
}