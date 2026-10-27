#include <iostream>
#include <vector>
#include <string>
#include <cctype>

std::vector<int> removeSmallest(const std::vector<int>& values) {
    if (values.empty()) {
        return {};
    }
    std::size_t minIndex = 0;
    for (std::size_t i = 1; i < values.size(); ++i) {
        if (values[i] < values[minIndex]) {
            minIndex = i;
        }
    }
    std::vector<int> result;
    result.reserve(values.size() - 1);
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i != minIndex) {
            result.push_back(values[i]);
        }
    }
    return result;
}

std::vector<int> parseIntegers(const std::string& text) {
    std::vector<int> values;
    std::size_t i = 0;
    while (i < text.size()) {
        if (std::isdigit(static_cast<unsigned char>(text[i])) ||
            (text[i] == '-' && i + 1 < text.size() &&
             std::isdigit(static_cast<unsigned char>(text[i + 1])))) {
            bool negative = false;
            if (text[i] == '-') {
                negative = true;
                ++i;
            }
            long long value = 0;
            while (i < text.size() && std::isdigit(static_cast<unsigned char>(text[i]))) {
                value = value * 10 + (text[i] - '0');
                ++i;
            }
            values.push_back(static_cast<int>(negative ? -value : value));
        } else {
            ++i;
        }
    }
    return values;
}

std::string formatIntegers(const std::vector<int>& values) {
    std::string out = "[";
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            out += ",";
        }
        out += std::to_string(values[i]);
    }
    out += "]";
    return out;
}

int main() {
    std::string input((std::istreambuf_iterator<char>(std::cin)),
                      std::istreambuf_iterator<char>());
    std::vector<int> values = parseIntegers(input);
    std::vector<int> result = removeSmallest(values);
    std::cout << formatIntegers(result) << std::endl;
    return 0;
}