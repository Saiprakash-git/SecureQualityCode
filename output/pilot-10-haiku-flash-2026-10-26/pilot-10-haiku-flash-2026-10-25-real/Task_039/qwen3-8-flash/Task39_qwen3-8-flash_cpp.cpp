#include <cstdlib>
#include <iostream>
#include <string>

static std::string quoteForShell(const std::string& filename)
{
#ifdef _WIN32
    std::string quoted = "\"";
    for (char c : filename) {
        if (c == '"') {
            quoted += "\\\"";
        } else {
            quoted += c;
        }
    }
    quoted += "\"";
    return quoted;
#else
    std::string quoted = "'";
    for (char c : filename) {
        if (c == '\'') {
            quoted += "'\\''";
        } else {
            quoted += c;
        }
    }
    quoted += "'";
    return quoted;
#endif
}

static int displayFileContents(const std::string& filename)
{
#ifdef _WIN32
    std::string command = "type " + quoteForShell(filename);
#else
    std::string command = "cat " + quoteForShell(filename);
#endif

    return std::system(command.c_str());
}

int main(int argc, char* argv[])
{
    std::string filename;

    if (argc > 1) {
        filename = argv[1];
    } else {
        std::cout << "Enter filename: ";
        std::getline(std::cin, filename);
    }

    if (filename.empty()) {
        return 0;
    }

    return displayFileContents(filename);
}