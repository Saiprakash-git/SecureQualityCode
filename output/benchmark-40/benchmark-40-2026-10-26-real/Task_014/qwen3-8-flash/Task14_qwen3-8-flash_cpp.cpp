#include <vector>

int findEvenIndex(const std::vector<int>& arr) {
    long long total = 0;
    for (int value : arr) {
        total += value;
    }

    long long leftSum = 0;

    for (std::size_t i = 0; i < arr.size(); ++i) {
        long long rightSum = total - leftSum - arr[i];

        if (leftSum == rightSum) {
            return static_cast<int>(i);
        }

        leftSum += arr[i];
    }

    return -1;
}