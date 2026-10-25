#include <iostream>
#include <string>
#include <cstdlib>

std::string buildCommand(const std::string& filename) {
#ifdef _WIN32
    return "type \"" + filename + "\"";
#else
    return "cat '" + filename + "'";
#endif
}

int runCommand(const std::string& command) {
    return std::system(command.c_str());
}

int main() {
    std::string filename;
    std::cout << "Enter filename: ";
    std::getline(std::cin, filename);

    if (filename.empty()) {
        std::cerr << "No filename provided." << std::endl;
        return 1;
    }

    int result = runCommand(buildCommand(filename));
    if (result != 0) {
        std::cerr << "Failed to display file: " << filename << std::endl;
        return 1;
    }

    return 0;
}