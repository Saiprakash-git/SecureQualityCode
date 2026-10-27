#include <string>
#include <sstream>

int scoreWord(const std::string& word) {
    int score = 0;
    for (char c : word) {
        if (c >= 'a' && c <= 'z') {
            score += c - 'a' + 1;
        }
    }
    return score;
}

std::string highestScoringWord(std::string str) {
    std::istringstream stream(str);
    std::string word;
    std::string bestWord;
    int bestScore = -1;

    while (stream >> word) {
        int currentScore = scoreWord(word);
        if (currentScore > bestScore) {
            bestScore = currentScore;
            bestWord = word;
        }
    }

    return bestWord;
}