#include <string>
#include <vector>

std::vector<std::string> towerBuilder(int nFloors) {
    std::vector<std::string> tower;
    for (int floor = 0; floor < nFloors; ++floor) {
        int spaces = nFloors - floor - 1;
        int stars = 2 * floor + 1;
        tower.push_back(std::string(spaces, ' ') + std::string(stars, '*') +
                        std::string(spaces, ' '));
    }
    return tower;
}