#include <string>
#include <vector>

std::string odd_or_even(const std::vector<int>& numbers) {
    bool odd = false;
    for (int number : numbers) {
        if (number % 2 != 0) {
            odd = !odd;
        }
    }
    return odd ? "odd" : "even";
}