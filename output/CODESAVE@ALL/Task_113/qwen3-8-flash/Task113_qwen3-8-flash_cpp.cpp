#include <iostream>
#include <string>
#include <vector>

std::string concatenateStrings(const std::vector<std::string>& strings) {
    std::string result;
    for (const std::string& s : strings) {
        result += s;
    }
    return result;
}

int main() {
    std::vector<std::string> strings;
    std::string input;

    while (std::cin >> input) {
        strings.push_back(input);
    }

    std::cout << concatenateStrings(strings);

    return 0;
}