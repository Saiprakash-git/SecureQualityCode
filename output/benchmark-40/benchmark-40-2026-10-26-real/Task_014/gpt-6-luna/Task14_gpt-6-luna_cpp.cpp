#include <vector>

int find_even_index(const std::vector<int>& numbers) {
    long long total = 0;
    for (int value : numbers) {
        total += value;
    }

    long long left = 0;
    for (int i = 0; i < static_cast<int>(numbers.size()); ++i) {
        total -= numbers[i];
        if (left == total) {
            return i;
        }
        left += numbers[i];
    }

    return -1;
}