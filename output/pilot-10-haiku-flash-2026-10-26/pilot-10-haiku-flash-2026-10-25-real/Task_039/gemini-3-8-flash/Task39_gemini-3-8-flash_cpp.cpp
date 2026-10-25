#include <iostream>
#include <string>
#include <cstdlib>

int displayFileContents(const std::string& filename) {
    if (filename.empty()) {
        return -1;
    }

#if defined(_WIN32) || defined(_WIN64)
    std::string command = "type \"" + filename + "\"";
#else
    std::string command = "cat \"" + filename + "\"";
#endif

    return std::system(command.c_str());
}

int main() {
    std::cout << "Enter filename: ";
    std::string filename;
    if (std::getline(std::cin, filename)) {
        return displayFileContents(filename);
    }
    return 1;
}