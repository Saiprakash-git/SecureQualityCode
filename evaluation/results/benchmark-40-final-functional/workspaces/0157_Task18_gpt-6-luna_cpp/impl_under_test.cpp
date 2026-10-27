#include <vector>

int binary_array_to_number(std::vector<int> arr) {
    int value = 0;
    for (int bit : arr) {
        value = value * 2 + bit;
    }
    return value;
}