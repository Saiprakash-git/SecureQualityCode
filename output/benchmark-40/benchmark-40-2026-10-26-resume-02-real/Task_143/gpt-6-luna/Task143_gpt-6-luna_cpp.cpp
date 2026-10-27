#include <algorithm>
#include <iostream>
#include <vector>

std::vector<int> sortArray(std::vector<int> numbers) {
    std::sort(numbers.begin(), numbers.end());
    return numbers;
}

int main() {
    int size;
    if (!(std::cin >> size)) {
        return 0;
    }

    std::vector<int> numbers(size);
    for (int& number : numbers) {
        std::cin >> number;
    }

    numbers = sortArray(numbers);

    for (int i = 0; i < size; ++i) {
        if (i > 0) {
            std::cout << ' ';
        }
        std::cout << numbers[i];
    }
    std::cout << '\n';

    return 0;
}