#include <iostream>
#include <string>
#include <cctype>
#include <optional>

namespace {
    constexpr std::size_t MAX_NAME_LENGTH = 100;
    constexpr int MIN_AGE = 0;
    constexpr int MAX_AGE = 150;

    bool isAllowedNameChar(unsigned char c) {
        return std::isalpha(c) || std::isspace(c) || c == '\'' || c == '-' || c == '.';
    }

    bool isSafeName(const std::string& name) {
        if (name.empty() || name.size() > MAX_NAME_LENGTH) {
            return false;
        }

        bool hasAlpha = false;

        for (unsigned char c : name) {
            if (std::iscntrl(c) || !isAllowedNameChar(c)) {
                return false;
            }

            if (std::isalpha(c)) {
                hasAlpha = true;
            }
        }

        return hasAlpha;
    }

    std::string normalizeName(const std::string& name) {
        std::string result;
        result.reserve(name.size());

        bool nextUpper = true;

        for (unsigned char c : name) {
            if (std::isspace(c)) {
                if (!result.empty() && result.back() != ' ') {
                    result.push_back(' ');
                }

                nextUpper = true;
            } else if (c == '\'' || c == '-' || c == '.') {
                result.push_back(static_cast<char>(c));
                nextUpper = true;
            } else {
                result.push_back(static_cast<char>(nextUpper ? std::toupper(c) : std::tolower(c)));
                nextUpper = false;
            }
        }

        if (!result.empty() && result.back() == ' ') {
            result.pop_back();
        }

        return result;
    }

    std::optional<int> parseAge(const std::string& input) {
        if (input.empty() || input.size() >