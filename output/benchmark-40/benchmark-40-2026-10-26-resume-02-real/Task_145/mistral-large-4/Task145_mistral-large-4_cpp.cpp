#include <iostream>
#include <vector>
#include <climits>

std::vector<int> maxSubarray(const std::vector<int>& arr) {
    if (arr.empty()) return {};

    int max_sum = INT_MIN;
    int current_sum = 0;
    int start = 0, end = 0, temp_start = 0;

    for (int i = 0; i < arr.size(); i++) {
        current_sum += arr[i];
        if (current_sum > max_sum) {
            max_sum = current_sum;
            start = temp_start;
            end = i;
        }
        if (current_sum < 0) {
            current_sum = 0;
            temp_start = i + 1;
        }
    }

    return std::vector<int>(arr.begin() + start, arr.begin() + end + 1);
}

int main() {
    int n;
    std::cin >> n;
    std::vector<int> arr(n);
    for (int i = 0; i < n; i++) {
        std::cin >> arr[i];
    }

    std::vector<int> result = maxSubarray(arr);
    int sum = 0;
    for (int x : result) {
        sum += x;
        std::cout << x << " ";
    }
    std::cout << "\n" << sum << std::endl;

    return 0;
}