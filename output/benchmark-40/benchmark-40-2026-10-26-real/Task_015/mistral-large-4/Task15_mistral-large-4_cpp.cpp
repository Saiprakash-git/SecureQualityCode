#include <vector>
#include <string>

std::vector<std::string> buildTower(int floors) {
    std::vector<std::string> tower;
    for (int i = 0; i < floors; ++i) {
        int spaces = floors - i - 1;
        int stars = 2 * i + 1;
        tower.push_back(std::string(spaces, ' ') + std::string(stars, '*') + std::string(spaces, ' '));
    }
    return tower;
}