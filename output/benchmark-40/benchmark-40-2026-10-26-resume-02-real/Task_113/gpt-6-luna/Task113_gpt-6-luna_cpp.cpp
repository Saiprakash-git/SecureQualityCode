#include <iostream>
#include <string>
#include <vector>

std::string concatenate(const std::vector<std::string>& strings) {
    std::string result;
    for (const std::string& value : strings) {
        result += value;
    }
    return result;
}

int main() {
    std::vector<std::string> strings;
    std::string value;

    while (std::cin >> value) {
        strings.push_back(value);
    }

    std::cout << concatenate(strings);
    return 0;
}