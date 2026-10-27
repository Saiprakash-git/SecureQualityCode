#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>
#include <cstring>

std::string copyString(const std::string& src) {
    return std::string(src);
}

std::string concatenateStrings(const std::string& a, const std::string& b) {
    return a + b;
}

std::string reverseString(const std::string& s) {
    std::string r(s.rbegin(), s.rend());
    return r;
}

std::string toUpperCase(const std::string& s) {
    std::string r = s;
    for (char& c : r) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return r;
}

std::string toLowerCase(const std::string& s) {
    std::string r = s;
    for (char& c : r) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return r;
}

std::string trim(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) ++start;
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
    return s.substr(start, end - start);
}

std::string replaceAll(const std::string& s, const std::string& from, const std::string& to) {
    if (from.empty()) return s;
    std::string result;
    size_t pos = 0;
    while (true) {
        size_t found = s.find(from, pos);
        if (found == std::string::npos) {
            result.append(s, pos, std::string::npos);
            break;
        }
        result.append(s, pos, found - pos);
        result.append(to);
        pos = found + from.size();
    }
    return result;
}

std::string removeChar(const std::string& s, char c) {
    std::string r;
    r.reserve(s.size());
    for (char ch : s) if (ch != c) r.push_back(ch);
    return r;
}

size_t countOccurrences(const std::string& s, const std::string& sub) {
    if (sub.empty()) return 0;
    size_t count = 0, pos = 0;
    while ((pos = s.find(sub, pos)) != std::string::npos) {
        ++count;
        pos += sub.size();
    }
    return count;
}

bool isPalindrome(const std::string& s) {
    std::string filtered;
    for (char c : s) {
        if (std::isalnum(static_cast<unsigned char>(c)))
            filtered.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    std::string rev(filtered.rbegin(), filtered.rend());
    return filtered == rev;
}

std::string copyWithCStrings(const char* src) {
    char buffer[1024];
    std::strncpy(buffer, src, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    return std::string(buffer);
}

int main() {
    std::string a = "  Hello, World  ";
    std::string b = "String Operations";

    std::cout << "Original A: [" << a << "]\n";
    std::cout << "Copy of A: [" << copyString(a) << "]\n";
    std::cout << "Concatenate A+B: [" << concatenateStrings(a, b) << "]\n";
    std::cout << "Trimmed A: [" << trim(a) << "]\n";
    std::cout << "Uppercase B: [" << toUpperCase(b) << "]\n";
    std::cout << "Lowercase B: [" << toLowerCase(b) << "]\n";
    std::cout << "Reversed B: [" << reverseString(b) << "]\n";
    std::cout << "Replace 'o' with '0' in A: [" << replaceAll(a, "o", "0") << "]\n";
    std::cout << "Remove spaces from B: [" << removeChar(b, ' ') << "]\n";
    std::cout << "Count 'o' in B: " << countOccurrences(b, "o") << "\n";
    std::cout << "Is 'A man, a plan, a canal: Panama' palindrome? "
              << (isPalindrome("A man, a plan, a canal: Panama") ? "yes" : "no") << "\n";
    std::cout << "C-string copy: [" << copyWithCStrings("C-style copy") << "]\n";

    return 0;
}