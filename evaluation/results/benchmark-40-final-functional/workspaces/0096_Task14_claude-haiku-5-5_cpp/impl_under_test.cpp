#include <iostream>
#include <vector>
#include <numeric>

int findEvenIndex(const std::vector<int>& arr) {
    long long total = std::accumulate(arr.begin(), arr.end(), 0LL);
    long long left = 0;
    for (size_t i = 0; i < arr.size(); ++i) {
        long long right = total - left - arr[i];
        if (left == right) {
            return static_cast<int>(i);
        }
        left += arr[i];
    }
    return -1;
}

int main() {
    std::vector<int> arr;
    int value;
    while (std::cin >> value) {
        arr.push_back(value);
    }
    std::cout << findEvenIndex(arr) << std::endl;
    return 0;
}