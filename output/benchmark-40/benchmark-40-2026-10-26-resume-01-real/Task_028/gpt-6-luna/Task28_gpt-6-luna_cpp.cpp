#include <string>
#include <vector>

std::string likes(const std::vector<std::string>& names) {
    const std::size_t count = names.size();

    if (count == 0) return "no one likes this";
    if (count == 1) return names[0] + " likes this";
    if (count == 2) return names[0] + " and " + names[1] + " like this";
    if (count == 3) return names[0] + ", " + names[1] + " and " + names[2] + " like this";

    return names[0] + ", " + names[1] + " and " +
           std::to_string(count - 2) + " others like this";
}