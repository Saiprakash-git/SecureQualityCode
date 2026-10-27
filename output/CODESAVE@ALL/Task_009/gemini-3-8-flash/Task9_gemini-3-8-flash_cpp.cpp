#include <iostream>
#include <vector>
#include <string>

std::string odd_or_even(const std::vector<int>& v) {
    long long sum = 0;
    for (int n : v) {
        sum += n;
    }
    return (sum % 2 != 0) ? "odd" : "even";
}

int main() {
    std::cout << odd_or_even({0}) << "\n";
    std::cout << odd_or_even({0, 1, 4}) << "\n";
    std::cout << odd_or_even({0, -1, -5}) << "\n";
    std::cout << odd_or_even({}) << "\n";
    return 0;
}