#include <string>
#include <vector>

std::string likes(const std::vector<std::string>& names) {
    size_t count = names.size();
    switch (count) {
        case 0:
            return "no one likes this";
        case 1:
            return names[0] + " likes this";
        case 2:
            return names[0] + " and " + names[1] + " like this";
        case 3:
            return names[0] + ", " + names[1] + " and " + names[2] + " like this";
        default:
            return names[0] + ", " + names[1] + " and " + std::to_string(count - 2) + " others like this";
    }
}

int main() {
    return 0;
}