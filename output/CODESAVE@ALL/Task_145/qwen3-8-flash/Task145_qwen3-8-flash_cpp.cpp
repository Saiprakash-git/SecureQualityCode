#include <iostream>
#include <vector>
#include <algorithm>
#include <limits>

using namespace std;

long long maxSubarraySum(const vector<int>& arr) {
    if (arr.empty()) {
        return 0;
    }

    long long maxSum = arr[0];
    long long currentSum = arr[0];

    for (size_t i = 1; i < arr.size(); ++i) {
        currentSum = max(static_cast<long long>(arr[i]), currentSum + arr[i]);
        maxSum = max(maxSum, currentSum);
    }

    return maxSum;
}

int main() {
    int n;

    if (!(cin >> n) || n <= 0) {
        cout << 0 << endl;
        return 0;
    }

    vector<int> arr(n);

    for (int i = 0; i < n; ++i) {
        cin >> arr[i];
    }

    cout << maxSubarraySum(arr) << endl;

    return 0;
}