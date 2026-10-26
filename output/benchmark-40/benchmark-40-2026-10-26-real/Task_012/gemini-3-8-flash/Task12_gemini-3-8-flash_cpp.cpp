#include <iostream>
#include <string>
#include <sstream>

std::string highestScoringWord(const std::string& str) {
    std::istringstream iss(str);
    std::string word;
    std::string best_word;
    int max_score = -1;

    while (iss >> word) {
        int current_score = 0;
        for (char c : word) {
            current_score += (c - 'a' + 1);
        }

        if (current_score > max_score) {
            max_score = current_score;
            best_word = word;
        }
    }

    return best_word;
}

int main() {
    std::string test = "man i need a taxi up to ubud";
    std::cout << highestScoringWord(test) << std::endl;
    return 0;
}