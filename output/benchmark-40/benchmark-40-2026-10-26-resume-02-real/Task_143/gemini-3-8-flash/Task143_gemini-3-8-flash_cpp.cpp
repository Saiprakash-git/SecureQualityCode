#include <iostream>
#include <vector>
#include <algorithm>

std::vector<int> sortAscending(std::vector<int> arr) {
    std::sort(arr.begin(), arr.end());
    return arr;
}

int main() {
    int n;
    if (!(std::cin >> n)) {
        return 0;
    }

    std::vector<int> arr(n);
    for (int i = 0; i < n; ++i) {
        std::cin >> arr[i];
    }

    std::vector<int> sortedArr = sortAscending(arr);

    for (int i = 0; i < n; ++i) {
        std::cout << sortedArr[i] << (i + 1 < n ? " " : "");
    }
    std::cout << "\n";

    return 0;
}