#include <iostream>
#include <vector>
#include <sstream>
#include <string>
#include <algorithm>

std::vector<int> sortAscending(std::vector<int> arr) {
    std::sort(arr.begin(), arr.end());
    return arr;
}

std::vector<int> parseIntegers(const std::string& line) {
    std::vector<int> result;
    std::istringstream iss(line);
    int value;
    while (iss >> value) {
        result.push_back(value);
    }
    return result;
}

int main() {
    std::string line;
    std::getline(std::cin, line);
    std::vector<int> numbers = parseIntegers(line);
    std::vector<int> sorted = sortAscending(numbers);
    for (size_t i = 0; i < sorted.size(); ++i) {
        if (i > 0) std::cout << ' ';
        std::cout << sorted[i];
    }
    std::cout << '\n';
    return 0;
}