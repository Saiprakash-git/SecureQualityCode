#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>
#include <vector>
#include <sstream>

std::string copyString(const std::string& source) {
    return std::string(source);
}

std::string concatenateStrings(const std::string& str1, const std::string& str2) {
    return str1 + str2;
}

std::string concatenateMultiple(const std::vector<std::string>& strings) {
    std::string result;
    for (const auto& str : strings) {
        result += str;
    }
    return result;
}

std::string reverseString(const std::string& str) {
    std::string reversed = str;
    std::reverse(reversed.begin(), reversed.end());
    return reversed;
}

std::string toUpperCase(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

std::string toLowerCase(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r\f\v");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r\f\v");
    return str.substr(first, (last - first + 1));
}

std::string replaceSubstring(std::string str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
    return str;
}

std::string removeSubstring(std::string str, const std::string& toRemove) {
    return replaceSubstring(str, toRemove, "");
}

bool containsSubstring(const std::string& str, const std::string& substr) {
    return str.find(substr) != std::string::npos;
}

std::vector<std::string> splitString(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

std::string joinStrings(const std::vector<std::string>& strings, const std::string& delimiter) {
    if (strings.empty()) return "";
    std::string result = strings[0];
    for (size_t i = 1; i < strings.size(); ++i) {
        result += delimiter + strings[i];
    }
    return result;
}

bool isPalindrome(const std::string& str) {
    std::string cleaned;
    for (char c : str) {
        if (std::isalnum(c)) {
            cleaned += std::tolower(c);
        }
    }
    std::string reversed = cleaned;
    std::reverse(reversed.begin(), reversed.end());
    return cleaned == reversed;
}

int countWords(const std::string& str) {
    std::istringstream iss(str);
    std::string word;
    int count = 0;
    while (iss >> word) {
        ++count;
    }
    return count;
}

std::string removeDuplicates(const std::string& str) {
    std::string result;
    for (char c : str) {
        if (result.find(c) == std::string::npos) {
            result += c;
        }
    }
    return result;
}

std::string capitalizeWords(const std::string& str) {
    std::string result = str;
    bool capitalizeNext = true;
    for (char& c : result) {
        if (std::isspace(c)) {
            capitalizeNext = true;
        } else if (capitalizeNext) {
            c = std::toupper(c);
            capitalizeNext = false;
        } else {
            c = std::tolower(c);
        }
    }
    return result;
}

std::string repeatString(const std::string& str, int times) {
    std::string result;
    for (int i = 0; i < times; ++i) {
        result += str;
    }
    return result;
}

std::string extractSubstring(const std::string& str, size_t start, size_t length) {
    if (start >= str.length()) return "";
    return str.substr(start, length);
}

int main() {
    std::string str1 = "Hello";
    std::string str2 = "World";

    std::cout << "Original strings: '" << str1 << "' and '" << str2 << "'" << std::endl;

    std::string copied = copyString(str1);
    std::cout << "Copied: '" << copied << "'" << std::endl;

    std::string concatenated = concatenateStrings(str1, str2);
    std::cout << "Concatenated: '" << concatenated << "'" << std::endl;

    std::string multi = concatenateMultiple({"C++", " ", "String", " ", "Operations"});
    std::cout << "Multiple concatenation: '" << multi << "'" << std::endl;

    std::string reversed = reverseString(concatenated);
    std::cout << "Reversed: '" << reversed << "'" << std::endl;

    std::string upper = toUpperCase(concatenated);
    std::cout << "Uppercase: '" << upper << "'" << std::endl;

    std::string lower = toLowerCase(concatenated);
    std::cout << "Lowercase: '" << lower << "'" << std::endl;

    std::string padded = "   Hello World   ";
    std::string trimmed = trim(padded);
    std::cout << "Trimmed: '" << trimmed << "'" << std::endl;

    std::string replaced = replaceSubstring("Hello World Hello", "Hello", "Hi");
    std::cout << "Replaced: '" << replaced << "'" << std::endl;

    std::string removed = removeSubstring("Hello World", "o");
    std::cout << "Removed 'o': '" << removed << "'" << std::endl;

    bool contains = containsSubstring("Hello World", "World");
    std::cout << "Contains 'World': " << (contains ? "Yes" : "No") << std::endl;

    std::vector<std::string> parts = splitString("apple,banana,cherry", ',');
    std::cout << "Split result: ";
    for (const auto& part : parts) {
        std::cout << "[" << part << "] ";
    }
    std::cout << std::endl;

    std::string joined = joinStrings({"Hello", "World", "C++"}, " - ");
    std::cout << "Joined: '" << joined << "'" << std::endl;

    std::string palindromeTest = "A man a plan a canal Panama";
    bool isPal = isPalindrome(palindromeTest);
    std::cout << "'" << palindromeTest << "' is palindrome: " << (isPal ? "Yes" : "No") << std::endl;

    int wordCount = countWords("Hello World this is a test");
    std::cout << "Word count: " << wordCount << std::endl;

    std::string withDups = "programming";
    std::string noDups = removeDuplicates(withDups);
    std::cout << "Removed duplicates from '" << withDups << "': '" << noDups << "'" << std::endl;

    std::string sentence = "hello world from c++";
    std::string capitalized = capitalizeWords(sentence);
    std::cout << "Capitalized: '" << capitalized << "'" << std::endl;

    std::string repeated = repeatString("abc", 3);
    std::cout << "Repeated 'abc' 3 times: '" << repeated << "'" << std::endl;

    std::string sub = extractSubstring("Hello World",