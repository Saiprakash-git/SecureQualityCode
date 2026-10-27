#include <string>
#include <sstream>

std::string high(const std::string& str) {
    std::istringstream input(str);
    std::string word, bestWord;
    int bestScore = -1;

    while (input >> word) {
        int score = 0;
        for (char letter : word) {
            score += letter - 'a' + 1;
        }
        if (score > bestScore) {
            bestScore = score;
            bestWord = word;
        }
    }

    return bestWord;
}