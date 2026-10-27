#include <iostream>
#include <vector>
#include <string>
#include <sstream>

std::string oddOrEven(const std::vector<int>& arr) {
    long long sum = 0;
    for (int x : arr) {
        sum += x;
    }
    return (sum % 2 == 0) ? "even" : "odd";
}

int main() {
    std::vector<int> input;
    std::string line;
    std::getline(std::cin, line);
    std::stringstream ss(line);
    std::string token;
    while (std::getline(ss, token, ',')) {
        std::stringstream ts(token);
        int v;
        if (ts >> v) {
            input.push_back(v);
        }
    }
    if (input.empty()) {
        input.push_back(0);
    }
    std::cout << "\"" << oddOrEven(input) << "\"" << std::endl;
    return 0;
}