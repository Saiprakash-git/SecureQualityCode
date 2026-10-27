#include <iostream>
#include <vector>
#include <string>

std::vector<std::string> buildTower(int nFloors) {
    std::vector<std::string> tower;
    if (nFloors <= 0) {
        return tower;
    }
    int width = 2 * nFloors - 1;
    for (int i = 0; i < nFloors; ++i) {
        int stars = 2 * i + 1;
        int pad = (width - stars) / 2;
        std::string row(pad, ' ');
        row += std::string(stars, '*');
        row += std::string(pad, ' ');
        tower.push_back(row);
    }
    return tower;
}

int main() {
    int n;
    if (!(std::cin >> n)) {
        return 0;
    }
    std::vector<std::string> tower = buildTower(n);
    for (const auto& row : tower) {
        std::cout << "\"" << row << "\"" << std::endl;
    }
    return 0;
}