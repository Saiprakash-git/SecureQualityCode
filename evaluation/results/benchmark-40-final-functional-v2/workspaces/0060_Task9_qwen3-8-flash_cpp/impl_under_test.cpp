#include <string>
#include <vector>

std::string odd_or_even(const std::vector<int>& arr) {
    bool odd = false;
    for (int value : arr) {
        if (value % 2 != 0) {
            odd = !odd;
        }
    }
    return odd ? "odd" : "even";
}