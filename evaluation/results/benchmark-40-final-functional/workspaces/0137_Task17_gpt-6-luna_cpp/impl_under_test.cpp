#include <cctype>
#include <string>

std::string toJadenCase(std::string phrase) {
    bool capitalizeNext = true;

    for (char& ch : phrase) {
        unsigned char value = static_cast<unsigned char>(ch);

        if (std::isspace(value)) {
            capitalizeNext = true;
        } else if (capitalizeNext) {
            ch = static_cast<char>(std::toupper(value));
            capitalizeNext = false;
        }
    }

    return phrase;
}