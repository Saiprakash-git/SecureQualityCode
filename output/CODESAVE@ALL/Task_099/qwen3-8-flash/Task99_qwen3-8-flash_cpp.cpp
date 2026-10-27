#include <iostream>
#include <fstream>
#include <regex>
#include <string>

bool lineMatches(const std::string& line) {
    static const std::regex pattern(R"(^\d+.*\.$)");
    return std::regex_match(line, pattern);
}

int processFile(const std::string& filename) {
    std::ifstream input(filename);

    if (!input.is_open()) {
        std::cerr << "Could not open file: " << filename << '\n';
        return 1;
    }

    std::string line;

    while (std::getline(input, line)) {
        if (lineMatches(line)) {
            std::cout << line << '\n';
        }
    }

    return 0;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>\n";
        return 1;
    }

    return processFile(argv[1]);
}