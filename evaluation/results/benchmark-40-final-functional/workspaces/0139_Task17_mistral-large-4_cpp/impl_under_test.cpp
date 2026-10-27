#include <string>
#include <cctype>

std::string toJadenCase(std::string str) {
    bool capitalize = true;
    for (char &c : str) {
        if (capitalize) {
            c = std::toupper(static_cast<unsigned char>(c));
            capitalize = false;
        } else {
            c = std::tolower(static_cast<unsigned char>(c));
        }
        if (c == ' ') {
            capitalize = true;
        }
    }
    return str;
}