#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <vector>

std::vector<std::string> findMatchingLines(const std::string& filename) {
    std::vector<std::string> result;
    std::ifstream inputFile(filename);
    std::string line;
    std::regex pattern(R"(^\d+.*\.$)");

    if (!inputFile.is_open()) {
        return result;
    }

    while (std::getline(inputFile, line)) {
        if (std::regex_match(line, pattern)) {
            result.push_back(line);
        }
    }

    inputFile.close();
    return result;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        return 1;
    }

    std::vector<std::string> matches = findMatchingLines(argv[1]);

    for (const auto& match : matches) {
        std::cout << match << '\n';
    }

    return 0;
}