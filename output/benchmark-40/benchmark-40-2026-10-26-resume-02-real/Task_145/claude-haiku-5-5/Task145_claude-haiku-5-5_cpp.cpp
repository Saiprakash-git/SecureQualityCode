#include <iostream>
#include <vector>
#include <sstream>
#include <string>
#include <limits>
#include <algorithm>

using namespace std;

struct MaxSubarrayResult {
    long long maxSum;
    size_t startIndex;
    size_t endIndex;
};

MaxSubarrayResult findMaxSubarray(const vector<long long>& nums) {
    MaxSubarrayResult best{numeric_limits<long long>::min(), 0, 0};
    if (nums.empty()) {
        best.maxSum = 0;
        return best;
    }

    long long currentSum = nums[0];
    size_t currentStart = 0;
    best.maxSum = nums[0];
    best.startIndex = 0;
    best.endIndex = 0;

    for (size_t i = 1; i < nums.size(); ++i) {
        if (currentSum < 0) {
            currentSum = nums[i];
            currentStart = i;
        } else {
            currentSum += nums[i];
        }
        if (currentSum > best.maxSum) {
            best.maxSum = currentSum;
            best.startIndex = currentStart;
            best.endIndex = i;
        }
    }
    return best;
}

vector<long long> parseIntegers(const string& line) {
    vector<long long> result;
    istringstream iss(line);
    long long value;
    while (iss >> value) {
        result.push_back(value);
    }
    return result;
}

int main() {
    cout << "Enter integers separated by spaces: ";
    string line;
    getline(cin, line);

    vector<long long> nums = parseIntegers(line);

    if (nums.empty()) {
        cout << "No input provided." << endl;
        return 0;
    }

    MaxSubarrayResult result = findMaxSubarray(nums);

    cout << "Maximum subarray sum: " << result.maxSum << endl;
    cout << "Subarray: ";
    for (size_t i = result.startIndex; i <= result.endIndex; ++i) {
        cout << nums[i];
        if (i < result.endIndex) cout << " ";
    }
    cout << endl;

    return 0;
}