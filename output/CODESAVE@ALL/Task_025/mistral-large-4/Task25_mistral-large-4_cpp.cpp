#include <vector>
#include <string>

std::vector<std::string> numberLines(const std::vector<std::string>& lines) {
    std::vector<std::string> result;
    result.reserve(lines.size());
    for (std::size_t i = 0; i < lines.size(); ++i) {
        result.push_back(std::to_string(i + 1) + ": " + lines[i]);
    }
    return result;
}