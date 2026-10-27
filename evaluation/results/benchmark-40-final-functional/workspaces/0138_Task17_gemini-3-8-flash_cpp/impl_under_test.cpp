#include <iostream>
#include <string>
#include <cctype>

std::string toJadenCase(std::string text) {
    bool capitalize_next = true;
    for (char &c : text) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            capitalize_next = true;
        } else if (capitalize_next) {
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            capitalize_next = false;
        }
    }
    return text;
}

int main() {
    std::string quote = "How can mirrors be real if our eyes aren't real";
    std::cout << toJadenCase(quote) << std::endl;
    return 0;
}