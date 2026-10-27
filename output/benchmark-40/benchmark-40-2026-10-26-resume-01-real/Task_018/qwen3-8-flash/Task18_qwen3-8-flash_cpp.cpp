#include <vector>

int binary_array_to_int(const std::vector<int>& bits) {
    int result = 0;
    for (int bit : bits) {
        result = result * 2 + (bit != 0 ? 1 : 0);
    }
    return result;
}