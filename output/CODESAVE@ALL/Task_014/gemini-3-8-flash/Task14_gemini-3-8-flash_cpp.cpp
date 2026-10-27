#include <vector>
#include <numeric>

int find_even_index(const std::vector<int>& numbers) {
    long long total_sum = std::accumulate(numbers.begin(), numbers.end(), 0LL);
    long long left_sum = 0;

    for (size_t i = 0; i < numbers.size(); ++i) {
        long long right_sum = total_sum - left_sum - numbers[i];
        if (left_sum == right_sum) {
            return static_cast<int>(i);
        }
        left_sum += numbers[i];
    }

    return -1;
}