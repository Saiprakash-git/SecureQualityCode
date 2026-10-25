#include <iostream>
#include <string>
#include <cstdlib>

void displayFileContents(const std::string& filename) {
    std::string command;
    #ifdef _WIN32
        command = "type \"" + filename + "\"";
    #else
        command = "cat \"" + filename + "\"";
    #endif
    std::system(command.c_str());
}

int main() {
    std::string filename;
    std::cout << "Enter filename: ";
    std::getline(std::cin, filename);
    displayFileContents(filename);
    return 0;
}