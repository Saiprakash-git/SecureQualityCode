#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <regex>

std::vector<std::string> findMatchingLines(const std::string& filePath) {
    std::vector<std::string> matchedLines;
    std::ifstream inputFile(filePath);

    if (!inputFile.is_open()) {
        std::cerr << "Error: Could not open file " << filePath << '\n';
        return matchedLines;
    }

    // Matches lines that start with one or more digits and end with a literal dot
    const std::regex pattern(R"(^\d+.*\.$)");
    std::string line;

    while (std::getline(inputFile, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (std::regex_match(line, pattern)) {
            matchedLines.push_back(line);
        }
    }

    return matchedLines;
}

int main(int argc, char* argv[]) {
    std::string filePath;

    if (argc > 1) {
        filePath = argv[1];
    } else {
        std::cout << "Enter file path: ";
        if (!std::getline(std::cin, filePath) || filePath.empty()) {
            return 1;
        }
    }

    std::vector<std::string> results = findMatchingLines(filePath);

    std::cout << "Matched lines (" << results.size() << "):\n";
    for (const auto& line : results) {
        std::cout << line << '\n';
    }

    return 0;
}