#include <vector>
#include <algorithm>

std::vector<int> remove_smallest(std::vector<int> numbers) {
    if (numbers.empty()) {
        return numbers;
    }

    auto smallest = std::min_element(numbers.begin(), numbers.end());
    numbers.erase(smallest);
    return numbers;
}