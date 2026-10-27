#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstddef>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace {

constexpr std::size_t kMaxLineLength = 256;
constexpr std::size_t kMaxValues = 100;
constexpr int kMaxFactorialInput = 20;

std::optional<std::string> readLine(std::istream& in, std::size_t maxLen) {
    std::string line;
    if (!std::getline(in, line)) {
        return std::nullopt;
    }
    if (line.size() > maxLen) {
        return std::nullopt;
    }
    return line;
}

std::string trim(const std::string& s) {
    std::size_t start = 0;
    std::size_t end = s.size();
    while (start < end && std::isspace(static_cast<unsigned char>(s[start]))) {
        ++start;
    }
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
        --end;
    }
    return s.substr(start, end - start);
}

bool isAllowedTextChar(char c) {
    unsigned char uc = static_cast<unsigned char>(c);
    return std::isprint(uc) != 0;
}

std::optional<std::string> validateText(const std::string& s, std::size_t maxLen) {
    if (s.empty() || s.size() > maxLen) {
        return std::nullopt;
    }
    for (char c : s) {
        if (!isAllowedTextChar(c)) {
            return std::nullopt;
        }
    }
    return s;
}

std::optional<long long> parseInteger(const std::string& token) {
    if (token.empty() || token.size() > 19) {
        return std::nullopt;
    }
    long long value = 0;
    const char* first = token.data();
    const char* last = token.data() + token.size();
    auto [ptr, ec] = std::from_chars(first, last, value);
    if (ec != std::errc() || ptr != last) {
        return std::nullopt;
    }
    return value;
}

std::optional<std::vector<long long>> parseIntegerList(const std::string& line) {
    std::vector<long long> values;
    std::string token;
    for (std::size_t i = 0; i <= line.size(); ++i) {
        char c = (i < line.size()) ? line[i] : ' ';
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (!token.empty()) {
                if (values.size() >= kMaxValues) {
                    return std::nullopt;
                }
                auto v = parseInteger(token);
                if (!v) {
                    return std::nullopt;
                }
                values.push_back(*v);
                token.clear();
            }
        } else {
            token.push_back(c);
        }
    }
    if (values.empty()) {
        return std::nullopt;
    }
    return values;
}

std::optional<long long> checkedSum(const std::vector<long long>& values) {
    long long total = 0;
    for (long long v : values) {
        if ((v > 0 && total > std::numeric_limits<long long>::max() - v) ||
            (v < 0 && total < std::numeric_limits<long long>::min() - v)) {
            return std::nullopt;
        }
        total += v;
    }
    return total;
}

std::optional<long long> factorial(int n) {
    if (n < 0 || n > kMaxFactorialInput) {
        return std::nullopt;
    }
    long long result = 1;
    for (int i = 2; i <= n; ++i) {
        if (result > std::numeric_limits<long long>::max() / i) {
            return std::nullopt;
        }
        result *= i;
    }
    return result;
}

std::string reverseText(const std::string& s) {
    return std::string(s.rbegin(), s.rend());
}

std::size_t countWords(const std::string& s) {
    std::size_t count = 0;
    bool inWord = false;
    for (char c : s) {
        bool space = std::isspace(static_cast<unsigned char>(c)) != 0;
        if (!space && !inWord) {
            ++count;
            inWord = true;
        } else if (space) {
            inWord = false;
        }
    }
    return count;
}

std::size_t countVowels(const std::string& s) {
    std::size_t count = 0;
    for (char c : s) {
        switch (std::tolower(static_cast<unsigned char>(c))) {
            case 'a': case 'e': case 'i': case 'o': case 'u':
                ++count;
                break;
            default:
                break;
        }
    }
    return count;
}

bool isPalindrome(const std::string& s) {
    std::string filtered;
    for (char c : s) {
        if (std::isalnum(static_cast<unsigned char>(c))) {
            filtered.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }
    }
    if (filtered.empty()) {
        return false;
    }
    return std::equal(filtered.begin(), filtered.begin() + filtered.size() / 2,
                      filtered.rbegin());
}

void printMenu() {
    std::cout << "\n=== Input Processing Menu ===\n"
              << "1) Sum integers\n"
              << "2) Factorial (0-" << kMaxFactorialInput << ")\n"
              << "3) Reverse text\n"
              << "4) Word and vowel count\n"
              << "5) Palindrome check\n"
              << "0) Exit\n"
              << "Choice: ";
}

std::optional<int> parseChoice(const std::string& s) {
    if (s.size() != 1 || !std::isdigit(static_cast<unsigned char>(s[0]))) {
        return std::nullopt;
    }
    return s[0] - '0';
}

void handleSum(std::istream& in) {
    std::cout << "Enter up to " << kMaxValues << " integers separated by spaces: ";
    auto line = readLine(in, kMaxLineLength);
    if (!line) {
        std::cout << "Error: invalid or too long input.\n";
        return;
    }
    auto values = parseIntegerList(trim(*line));
    if (!values) {
        std::cout << "Error: input must contain valid integers (max " << kMaxValues << ").\n";
        return;
    }
    auto total = checkedSum(*values);
    if (!total) {
        std::cout << "Error: sum overflows the supported range.\n";
        return;
    }
    std::cout << "Sum: " << *total << "\n";
}

void handleFactorial(std::istream& in) {
    std::cout << "Enter a non-negative integer (0-" << kMaxFactorialInput << "): ";
    auto line = readLine(in, kMaxLineLength);
    if (!line) {
        std::cout << "Error: invalid or too long input.\n";
        return;
    }
    auto n = parseInteger(trim(*line));
    if (!n || *n < 0 || *n > kMaxFactorialInput) {
        std::cout << "Error: value out of allowed range.\n";
        return;
    }
    auto result = factorial(static_cast<int>(*n));
    if (!result) {
        std::cout << "Error: factorial overflows.\n";
        return;
    }
    std::cout << "Factorial: " << *result << "\n";
}

void handleText(std::istream& in, int choice) {
    std::cout << "Enter text (printable ASCII, max " << kMaxLineLength << " chars): ";
    auto line = readLine(in, kMaxLineLength);
    if (!line) {
        std::cout << "Error: invalid or too long input.\n";
        return;
    }
    auto text = validateText(*line, kMaxLineLength);
    if (!text) {
        std::cout << "Error: text must be non-empty printable characters.\n";
        return;
    }
    switch (choice) {
        case 3:
            std::cout << "Reversed: " << reverseText(*text) << "\n";
            break;
        case 4:
            std::cout << "Words: " << countWords(*text)
                      << ", Vowels: " << countVowels(*text) << "\n";
            break;
        case 5:
            std::cout << (isPalindrome(*text) ? "Palindrome: yes\n" : "Palindrome: no\n");
            break;
        default:
            std::cout << "Error: unknown operation.\n";
            break;
    }
}

}  // namespace

int main() {
    std::cin.exceptions(std::ios::goodbit);
    while (true) {
        printMenu();
        auto choiceLine = readLine(std::cin, 8);
        if (!choiceLine) {
            std::cout << "\nInput closed or invalid. Exiting.\n";
            break;
        }
        auto choice = parseChoice(trim(*choiceLine));
        if (!choice) {
            std::c