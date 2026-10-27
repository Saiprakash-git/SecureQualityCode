#include <vector>
#include <algorithm>

std::vector<int> removeSmallest(const std::vector<int>& numbers) {
    if (numbers.empty()) {
        return {};
    }

    auto smallest = std::min_element(numbers.begin(), numbers.end());

    std::vector<int> result;
    result.reserve(numbers.size() - 1);

    for (auto it = numbers.begin(); it != numbers.end(); ++it) {
        if (it != smallest) {
            result.push_back(*it);
        }
    }

    return result;
}

std::vector<int> remove_smallest(const std::vector<int>& numbers) {
    return removeSmallest(numbers);
}