#include <string>
#include <cctype>

std::string toJadenCase(const std::string& str) {
    std::string result;
    result.reserve(str.size());

    bool capitalizeNext = true;

    for (unsigned char ch : str) {
        if (std::isspace(ch)) {
            result.push_back(static_cast<char>(ch));
            capitalizeNext = true;
        } else {
            if (capitalizeNext) {
                result.push_back(static_cast<char>(std::toupper(ch)));
                capitalizeNext = false;
            } else {
                result.push_back(static_cast<char>(ch));
            }
        }
    }

    return result;
}