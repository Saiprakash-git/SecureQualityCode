#include <vector>
#include <string>

std::string oddOrEven(const std::vector<int>& nums) {
    int sum = 0;
    for (int num : nums) {
        sum += num;
    }
    return (sum % 2 != 0) ? "odd" : "even";
}