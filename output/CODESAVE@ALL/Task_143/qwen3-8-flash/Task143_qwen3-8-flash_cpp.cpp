#include <iostream>
#include <vector>
#include <algorithm>

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    int n;
    if (!(std::cin >> n)) {
        return 0;
    }

    std::vector<int> arr(n);
    for (int i = 0; i < n; ++i) {
        std::cin >> arr[i];
    }

    std::sort(arr.begin(), arr.end());

    for (int i = 0; i < n; ++i) {
        if (i > 0) {
            std::cout << ' ';
        }
        std::cout << arr[i];
    }

    std::cout << '\n';
    return 0;
}