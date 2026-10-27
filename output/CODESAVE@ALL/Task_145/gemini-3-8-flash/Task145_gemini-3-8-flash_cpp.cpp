#include <iostream>
#include <vector>
#include <climits>

struct SubarrayResult {
    long long max_sum;
    int start_index;
    int end_index;
    std::vector<int> subarray;
};

SubarrayResult findMaxSubarray(const std::vector<int>& nums) {
    if (nums.empty()) {
        return {0, -1, -1, {}};
    }

    long long max_so_far = nums[0];
    long long current_max = nums[0];
    int start = 0;
    int end = 0;
    int temp_start = 0;

    for (size_t i = 1; i < nums.size(); ++i) {
        if (nums[i] > current_max + nums[i]) {
            current_max = nums[i];
            temp_start = static_cast<int>(i);
        } else {
            current_max += nums[i];
        }

        if (current_max > max_so_far) {
            max_so_far = current_max;
            start = temp_start;
            end = static_cast<int>(i);
        }
    }

    std::vector<int> sub(nums.begin() + start, nums.begin() + end + 1);
    return {max_so_far, start, end, sub};
}

int main() {
    int n;
    if (!(std::cin >> n) || n <= 0) {
        return 0;
    }

    std::vector<int> nums(n);
    for (int i = 0; i < n; ++i) {
        std::cin >> nums[i];
    }

    SubarrayResult result = findMaxSubarray(nums);

    std::cout << "Maximum Sum: " << result.max_sum << "\n";
    std::cout << "Subarray: [";
    for (size_t i = 0; i < result.subarray.size(); ++i) {
        std::cout << result.subarray[i] << (i + 1 < result.subarray.size() ? ", " : "");
    }
    std::cout << "]\n";
    std::cout << "Indices: [" << result.start_index << ", " << result.end_index << "]\n";

    return 0;
}