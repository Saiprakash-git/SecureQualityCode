#include <fstream>
#include <iostream>
#include <regex>
#include <string>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <file>\n";
        return 1;
    }

    std::ifstream file(argv[1]);
    if (!file) {
        std::cerr << "Error: could not open file\n";
        return 1;
    }

    const std::regex pattern(R"(^[0-9]+.*\.$)");
    std::string line;

    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (std::regex_match(line, pattern)) {
            std::cout << line << '\n';
        }
    }

    return 0;
}