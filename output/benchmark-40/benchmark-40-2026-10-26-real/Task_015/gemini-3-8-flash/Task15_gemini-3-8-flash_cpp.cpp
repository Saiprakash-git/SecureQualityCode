#include <iostream>
#include <string>
#include <vector>

std::vector<std::string> towerBuilder(unsigned int nFloors) {
    std::vector<std::string> tower;
    tower.reserve(nFloors);
    for (unsigned int i = 0; i < nFloors; ++i) {
        std::string spaces(nFloors - 1 - i, ' ');
        std::string stars(2 * i + 1, '*');
        tower.push_back(spaces + stars + spaces);
    }
    return tower;
}

int main() {
    unsigned int floors = 6;
    std::vector<std::string> tower = towerBuilder(floors);
    for (const auto& floor : tower) {
        std::cout << "\"" << floor << "\"\n";
    }
    return 0;
}