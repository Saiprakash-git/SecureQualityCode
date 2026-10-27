#include <iostream>
#include <vector>
#include <cassert>

int binaryArrayToInt(const std::vector<int>& bits) {
    int result = 0;
    for (int bit : bits) {
        result = (result << 1) | (bit & 1);
    }
    return result;
}

int main() {
    assert(binaryArrayToInt({0, 0, 0, 1}) == 1);
    assert(binaryArrayToInt({0, 0, 1, 0}) == 2);
    assert(binaryArrayToInt({0, 1, 0, 1}) == 5);
    assert(binaryArrayToInt({1, 0, 0, 1}) == 9);
    assert(binaryArrayToInt({0, 1, 1, 0}) == 6);
    assert(binaryArrayToInt({1, 1, 1, 1}) == 15);
    assert(binaryArrayToInt({1, 0, 1, 1}) == 11);
    std::cout << binaryArrayToInt({1, 0, 1, 1}) << std::endl;
    return 0;
}