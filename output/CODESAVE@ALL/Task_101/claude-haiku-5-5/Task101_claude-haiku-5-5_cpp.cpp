#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/stat.h>

bool createExecutableScript(const std::string& filename, const std::string& content) {
    std::ofstream out(filename);
    if (!out) {
        return false;
    }
    out << content;
    out.close();
    if (!out) {
        return false;
    }
    return chmod(filename.c_str(), S_IRUSR | S_IWUSR | S_IXUSR |
                                   S_IRGRP | S_IXGRP |
                                   S_IROTH | S_IXOTH) == 0;
}

int main() {
    const std::string filename = "script.sh";
    const std::string content = "#!/bin/sh\necho \"Hello from script.sh\"\n";

    if (!createExecutableScript(filename, content)) {
        std::cerr << "Failed to create or make executable: " << filename << "\n";
        return 1;
    }

    std::cout << "Created and made executable: " << filename << "\n";
    return 0;
}