#include <vector>
#include <algorithm>
#include <iostream>

std::vector<int> removeSmallest(const std::vector<int>& numbers) {
    if (numbers.empty()) {
        return {};
    }
    
    std::vector<int> result = numbers;
    result.erase(std::min_element(result.begin(), result.end()));
    return result;
}

int main() {
    std::vector<int> ex1 = {1, 2, 3, 4, 5};
    std::vector<int> res1 = removeSmallest(ex1);

    std::vector<int> ex2 = {5, 3, 2, 1, 4};
    std::vector<int> res2 = removeSmallest(ex2);

    std::vector<int> ex3 = {2, 2, 1, 2, 1};
    std::vector<int> res3 = removeSmallest(ex3);

    return 0;
}