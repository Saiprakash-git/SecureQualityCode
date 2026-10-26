#include <vector>
#include <iostream>

int binaryArrayToInt(const std::vector<int>& binaryArray) {
    int result = 0;
    for (int bit : binaryArray) {
        result = (result << 1) | bit;
    }
    return result;
}

int main() {
    std::vector<std::vector<int>> testCases = {
        {0, 0, 0, 1},
        {0, 0, 1, 0},
        {0, 1, 0, 1},
        {1, 0, 0, 1},
        {0, 0, 1, 0},
        {0, 1, 1, 0},
        {1, 1, 1, 1},
        {1, 0, 1, 1}
    };

    for (const auto& testCase : testCases) {
        std::cout << "Testing: [";
        for (size_t i = 0; i < testCase.size(); ++i) {
            std::cout << testCase[i];
            if (i < testCase.size() - 1) {
                std::cout << ", ";
            }
        }
        std::cout << "] ==> " << binaryArrayToInt(testCase) << std::endl;
    }

    return 0;
}