#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <sstream>

std::string string_copy(const std::string& source) {
    return std::string(source);
}

std::string string_concat(const std::string& first, const std::string& second) {
    return first + second;
}

std::string string_join(const std::vector<std::string>& tokens, const std::string& delimiter) {
    std::string result;
    for (size_t i = 0; i < tokens.size(); ++i) {
        result += tokens[i];
        if (i + 1 < tokens.size()) {
            result += delimiter;
        }
    }
    return result;
}

std::string to_uppercase(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return str;
}

std::string to_lowercase(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return str;
}

std::string reverse_string(std::string str) {
    std::reverse(str.begin(), str.end());
    return str;
}

std::string trim_left(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r\f\v");
    return (start == std::string::npos) ? "" : str.substr(start);
}

std::string trim_right(const std::string& str) {
    size_t end = str.find_last_not_of(" \t\n\r\f\v");
    return (end == std::string::npos) ? "" : str.substr(0, end + 1);
}

std::string trim(const std::string& str) {
    return trim_right(trim_left(str));
}

std::string replace_all(std::string str, const std::string& target, const std::string& replacement) {
    if (target.empty()) {
        return str;
    }
    size_t pos = 0;
    while ((pos = str.find(target, pos)) != std::string::npos) {
        str.replace(pos, target.length(), replacement);
        pos += replacement.length();
    }
    return str;
}

std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream token_stream(str);
    while (std::getline(token_stream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

bool is_palindrome(const std::string& str) {
    std::string sanitized;
    for (unsigned char c : str) {
        if (std::isalnum(c)) {
            sanitized += static_cast<char>(std::tolower(c));
        }
    }
    size_t n = sanitized.length();
    for (size_t i = 0; i < n / 2; ++i) {
        if (sanitized[i] != sanitized[n - 1 - i]) {
            return false;
        }
    }
    return true;
}

int main() {
    std::string original = "  Hello, World! Welcome to C++ String Operations.  ";

    std::string copied = string_copy(original);
    std::cout << "Original: [" << original << "]\n";
    std::cout << "Copied:   [" << copied << "]\n\n";

    std::string trimmed = trim(original);
    std::cout << "Trimmed:  [" << trimmed << "]\n";

    std::string upper = to_uppercase(trimmed);
    std::string lower = to_lowercase(trimmed);
    std::cout << "Uppercase: " << upper << "\n";
    std::cout << "Lowercase: " << lower << "\n\n";

    std::string reversed = reverse_string(trimmed);
    std::cout << "Reversed: " << reversed << "\n\n";

    std::string s1 = "Super";
    std::string s2 = "Structure";
    std::string combined = string_concat(s1, s2);
    std::cout << "Concatenation: " << combined << "\n\n";

    std::string replaced = replace_all(trimmed, "C++", "Modern C++");
    std::cout << "Replacement: " << replaced << "\n\n";

    std::string csv_data = "Apple,Orange,Banana,Mango,Pineapple";
    std::vector<std::string> fruits = split(csv_data, ',');
    std::cout << "Split tokens:\n";
    for (const auto& fruit : fruits) {
        std::cout << " - " << fruit << "\n";
    }

    std::string joined = string_join(fruits, " -> ");
    std::cout << "Joined back: " << joined << "\n\n";

    std::string pal1 = "A man, a plan, a canal: Panama";
    std::string pal2 = "Not a palindrome";
    std::cout << "\"" << pal1 << "\" is palindrome? " 
              << (is_palindrome(pal1) ? "true" : "false") << "\n";
    std::cout << "\"" << pal2 << "\" is palindrome? " 
              << (is_palindrome(pal2) ? "true" : "false") << "\n";

    return 0;
}