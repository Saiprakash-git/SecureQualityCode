```cpp
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <regex>

std::vector<std::string> readLines(const std::string& path, bool& ok) {
    std::vector<std::string> lines;
    std::ifstream file(path);
    ok = file.is_open();
    if (!ok) {
        return lines;
    }
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back(line);
    }
    return lines;
}

std::vector<std::string> matchLines(const std::vector<std::string>& lines, const std::regex& pattern) {
    std::vector<std::string> matches;
    for (const auto& line : lines) {
        if (std::regex_match(line, pattern)) {
            matches.push_back(line);
        }
    }
    return matches;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>\n";
        return 1;
    }

    bool ok = false;
    std::vector<std::string> lines = readLines(argv[1], ok);
    if (!ok) {
        std::cerr << "Error: cannot open file '" << argv[1] << "'\n";
        return 1;
    }

    const std::regex pattern(R"(^[0-9]+.*\.$)");
    std::vector<std::string> matches = matchLines(lines, pattern);

    for (const auto& m : matches) {
        std::cout << m << '\n';
    }

    return 0;
}
```