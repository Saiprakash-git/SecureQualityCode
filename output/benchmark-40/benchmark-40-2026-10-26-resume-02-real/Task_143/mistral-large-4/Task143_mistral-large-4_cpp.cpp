#include <iostream>
#include <vector>
#include <algorithm>

std::vector<int> sortArray(std::vector<int> arr) {
    std::sort(arr.begin(), arr.end());
    return arr;
}

int main() {
    int n;
    std::cin >> n;
    std::vector<int> arr(n);
    for (int i = 0; i < n; i++) {
        std::cin >> arr[i];
    }
    std::vector<int> sorted = sortArray(arr);
    for (int i = 0; i < sorted.size(); i++) {
        std::cout << sorted[i];
        if (i < sorted.size() - 1) std::cout << " ";
    }
    std::cout << std::endl;
    return 0;
}