#include <iostream>
#include <string>
#include <cctype>

bool isPangram(const std::string& str) {
    int seen = 0;
    const int target = (1 << 26) - 1;

    for (unsigned char c : str) {
        if (std::isalpha(c)) {
            seen |= (1 << (std::tolower(c) - 'a'));
            if (seen == target) {
                return true;
            }
        }
    }

    return seen == target;
}

int main() {
    std::string test1 = "The quick brown fox jumps over the lazy dog";
    std::string test2 = "This is not a pangram.";

    std::cout << std::boolalpha;
    std::cout << isPangram(test1) << "\n";
    std::cout << isPangram(test2) << "\n";

    return 0;
}