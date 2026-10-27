#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <algorithm>

std::string copyString(const std::string& input) {
    return input;
}

std::string concatenateStrings(const std::string& first, const std::string& second) {
    return first + second;
}

std::string concatenateMany(const std::vector<std::string>& parts) {
    std::string result;
    for (const std::string& part : parts) {
        result += part;
    }
    return result;
}

std::size_t stringLength(const std::string& input) {
    return input.size();
}

int compareStrings(const std::string& first, const std::string& second) {
    if (first < second) return -1;
    if (first > second) return 1;
    return 0;
}

std::string reverseString(const std::string& input) {
    std::string result = input;
    std::reverse(result.begin(), result.end());
    return result;
}

std::string toUpperString(const std::string& input) {
    std::string result;
    result.reserve(input.size());
    for (unsigned char ch : input) {
        result += static_cast<char>(std::toupper(ch));
    }
    return result;
}

std::string toLowerString(const std::string& input) {
    std::string result;
    result.reserve(input.size());
    for (unsigned char ch : input) {
        result += static_cast<char>(std::tolower(ch));
    }
    return result;
}

std::string substring(const std::string& input, std::size_t start, std::size_t length) {
    if (start >= input.size()) {
        return "";
    }
    return input.substr(start, length);
}

std::string replaceSubstring(const std::string& input, const std::string& target, const std::string& replacement) {
    if (target.empty()) {
        return input;
    }

    std::string result;
    std::size_t position = 0;

    while (position < input.size()) {
        std::size_t found = input.find(target, position);

        if (found == std::string::npos) {
            result += input.substr(position);
            break;
        }

        result += input.substr(position, found - position);
        result += replacement;
        position = found + target.size();
    }

    return result;
}

std::string trimString(const std::string& input) {
    std::size_t start = 0;
    std::size_t end = input.size();

    while (start < end && std::isspace(static_cast<unsigned char>(input[start]))) {
        ++start;
    }

    while (end > start && std::isspace(static_cast<unsigned char>(input[end - 1]))) {
        --end;
    }

    return input.substr(start, end - start);
}

std::string repeatString(const std::string& input, int times) {
    if (times <= 0) {
        return "";
    }

    std::string result;
    for (int i = 0; i < times; ++i) {
        result += input;
    }
    return result;
}

std::string joinStrings(const std::vector<std::string>& parts, const std::string& delimiter) {
    std::string result;

    for (std::size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) {
            result += delimiter;
        }
        result += parts[i];
    }

    return result;
}

std::vector<std::string> splitString(const std::string& input, char delimiter) {
    std::vector<std::string> parts;
    std::string current;

    for (char ch : input) {
        if (ch == delimiter) {
            parts.push_back(current);
            current.clear();
        } else {
            current += ch;
        }
    }

    parts.push_back(current);
    return parts;
}

bool startsWith(const std::string& input, const std::string& prefix) {
    if (prefix.size() > input.size()) {
        return false;
    }
    return input.compare(0, prefix.size(), prefix) == 0;
}

bool endsWith(const std::string& input, const std::string& suffix) {
    if (suffix.size() > input.size()) {
        return false;
    }
    return input.compare(input.size() - suffix.size(), suffix.size(), suffix) == 0;
}

int main() {
    std::string original = "  Hello, World!  ";

    std::cout << "Original: \"" << original << "\"\n";
    std::cout << "Copied: \"" << copyString(original) << "\"\n";
    std::cout << "Trimmed: \"" << trimString(original) << "\"\n";
    std::cout << "Length: " << stringLength(original) << "\n";
    std::cout << "Concatenated: \"" << concatenateStrings("Hello, ", "World!") << "\"\n";
    std::cout << "Concatenated many: \"" << concatenateMany({"C++", " ", "string", " ", "operations"}) << "\"\n";
    std::cout << "Reversed: \"" << reverseString("abcdef") << "\"\n";
    std::cout << "Uppercase: \"" << toUpperString("hello") << "\"\n";
    std::cout << "Lowercase: \"" << toLowerString("WORLD") << "\"\n";
    std::cout << "Substring: \"" << substring("abcdefghij", 2, 5) << "\"\n";
    std::cout << "Replaced: \"" << replaceSubstring("foo bar foo", "foo", "baz") << "\"\n";
    std::cout << "Repeated: \"" << repeatString("ab", 3) << "\"\n";
    std::cout << "Joined: \"" << joinStrings({"apple", "banana", "cherry"}, ", ") << "\"\n";

    std::vector<std::string> splitResult = splitString("one,two,three,four", ',');
    std::cout << "Split: ";
    for (std::size_t i = 0; i < splitResult.size(); ++i) {
        std::cout << "\"" << splitResult[i] << "\"";
        if (i + 1 < splitResult.size()) {
            std::cout << ", ";
        }
    }
    std::cout << "\n";

    std::cout << "Compare \"abc\" and \"abd\": " << compareStrings("abc", "abd") << "\n";
    std::cout << "Starts with \"Hello\": " << (startsWith("Hello, World!", "Hello") ? "true" : "false") << "\n";
    std::cout << "Ends with \"World!\": " << (endsWith("Hello, World!", "World!") ? "true" : "false") << "\n";

    return 0;
}