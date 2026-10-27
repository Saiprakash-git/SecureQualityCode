#include <string>
#include <sstream>

std::string highestScoringWord(const std::string& input) {
    std::istringstream iss(input);
    std::string word;
    std::string bestWord;
    int maxScore = -1;

    while (iss >> word) {
        int score = 0;
        for (char c : word) {
            score += (c - 'a' + 1);
        }
        if (score > maxScore) {
            maxScore = score;
            bestWord = word;
        }
    }

    return bestWord;
}