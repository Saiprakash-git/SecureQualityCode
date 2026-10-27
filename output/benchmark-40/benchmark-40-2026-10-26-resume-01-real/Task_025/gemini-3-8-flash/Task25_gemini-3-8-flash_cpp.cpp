#include <vector>
#include <string>

std::vector<std::string> number(const std::vector<std::string>& lines) {
    std::vector<std::string> result;
    result.reserve(lines.size());
    for (size_t i = 0; i < lines.size(); ++i) {
        result.push_back(std::to_string(i + 1) + ": " + lines[i]);
    }
    return result;
}

int main() {
    return 0;
}