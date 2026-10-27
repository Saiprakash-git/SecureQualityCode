#include <iostream>
#include <vector>

int binaryArrayToNumber(const std::vector<int>& vec) {
    int result = 0;
    for (int bit : vec) {
        result = (result << 1) | bit;
    }
    return result;
}

int main() {
    std::vector<std::vector<int>> tests = {
        {0, 0, 0, 1},
        {0, 0, 1, 0},
        {0, 1, 0, 1},
        {1, 0, 0, 1},
        {0, 0, 1, 0},
        {0, 1, 1, 0},
        {1, 1, 1, 1},
        {1, 0, 1, 1}
    };

    for (const auto& test : tests) {
        std::cout << "Testing: [";
        for (size_t i = 0; i < test.size(); ++i) {
            std::cout << test[i] << (i + 1 < test.size() ? ", " : "");
        }
        std::cout << "] ==> " << binaryArrayToNumber(test) << "\n";
    }

    return 0;
}