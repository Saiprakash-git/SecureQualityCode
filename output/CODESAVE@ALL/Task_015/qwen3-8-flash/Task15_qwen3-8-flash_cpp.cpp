#include <vector>
#include <string>

class Tower {
public:
    static std::vector<std::string> buildTower(int floors) {
        std::vector<std::string> tower;

        if (floors <= 0) {
            return tower;
        }

        int width = 2 * floors - 1;

        for (int i = 0; i < floors; ++i) {
            int stars = 2 * i + 1;
            int spaces = (width - stars) / 2;

            std::string line;
            line.append(spaces, ' ');
            line.append(stars, '*');
            line.append(spaces, ' ');

            tower.push_back(line);
        }

        return tower;
    }
};

std::vector<std::string> buildTower(int floors) {
    return Tower::buildTower(floors);
}