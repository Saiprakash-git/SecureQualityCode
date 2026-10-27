#include <cctype>
#include <iostream>
#include <string>

namespace {
constexpr std::size_t MAX_LINE_LENGTH = 4096;

bool readLine(std::string& line) {
    line.clear();
    bool overflow = false;
    char ch;

    while (std::cin.get(ch)) {
        if (ch == '\n') {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            return !overflow;
        }

        if (line.size() < MAX_LINE_LENGTH) {
            line.push_back(ch);
        } else {
            overflow = true;
        }
    }

    if (overflow) {
        return false;
    }
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
    return !line.empty();
}

std::string toUppercase(const std::string& input) {
    std::string result = input;
    for (char& ch : result) {
        if (ch >= 'a' && ch <= 'z') {
            ch = static_cast<char>(ch - 'a' + 'A');
        }
    }
    return result;
}

std::string toLowercase(const std::string& input) {
    std::string result = input;
    for (char& ch : result) {
        if (ch >= 'A' && ch <= 'Z') {
            ch = static_cast<char>(ch - 'A' + 'a');
        }
    }
    return result;
}

std::string reverseText(const std::string& input) {
    return std::string(input.rbegin(), input.rend());
}

std::size_t countWords(const std::string& input) {
    std::size_t count = 0;
    bool inWord = false;

    for (unsigned char ch : input) {
        if (std::isspace(ch)) {
            inWord = false;
        } else if (!inWord) {
            ++count;
            inWord = true;
        }
    }

    return count;
}

std::string trimText(const std::string& input) {
    std::size_t first = 0;
    while (first < input.size() &&
           std::isspace(static_cast<unsigned char>(input[first]))) {
        ++first;
    }

    std::size_t last = input.size();
    while (last > first &&
           std::isspace(static_cast<unsigned char>(input[last - 1]))) {
        --last;
    }

    return input.substr(first, last - first);
}
}  // namespace

int main() {
    std::string operation;
    std::string text;

    if (!readLine(operation) || operation.empty() ||
        !readLine(text)) {
        std::cerr << "Invalid input.\n";
        return 1;
    }

    if (operation == "uppercase") {
        std::cout << toUppercase(text) << '\n';
    } else if (operation == "lowercase") {
        std::cout << toLowercase(text) << '\n';
    } else if (operation == "reverse") {
        std::cout << reverseText(text) << '\n';
    } else if (operation == "word-count") {
        std::cout << countWords(text) << '\n';
    } else if (operation == "character-count") {
        std::cout << text.size() << '\n';
    } else if (operation == "trim") {
        std::cout << trimText(text) << '\n';
    } else {
        std::cerr << "Unsupported operation.\n";
        return 1;
    }

    return 0;
}