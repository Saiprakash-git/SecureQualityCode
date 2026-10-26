#include <iostream>
#include <sstream>
#include <string>

std::string high(const std::string& s) {
    std::istringstream iss(s);
    std::string word;
    std::string best;
    int bestScore = -1;
    while (iss >> word) {
        int score = 0;
        for (char c : word) {
            if (c >= 'a' && c <= 'z') {
                score += c - 'a' + 1;
            }
        }
        if (score > bestScore) {
            bestScore = score;
            best = word;
        }
    }
    return best;
}

int main() {
    std::string line;
    std::getline(std::cin, line);
    std::cout << high(line) << std::endl;
    return 0;
}