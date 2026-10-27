#include <vector>
#include <algorithm>

std::vector<int> removeSmallest(const std::vector<int>& numbers) {
    if (numbers.empty()) {
        return {};
    }
    auto minIt = std::min_element(numbers.begin(), numbers.end());
    std::vector<int> result = numbers;
    result.erase(result.begin() + (minIt - numbers.begin()));
    return result;
}