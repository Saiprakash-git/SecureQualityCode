#include <iostream>
#include <fstream>
#include <string>
#include <sys/stat.h>

bool createExecutableScript(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    file << "#!/bin/bash\n";
    file << "echo \"Hello from script.sh\"\n";
    file.close();

    mode_t mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
    if (chmod(filename.c_str(), mode) != 0) {
        return false;
    }

    return true;
}

int main() {
    std::string filename = "script.sh";

    if (createExecutableScript(filename)) {
        std::cout << "Successfully created " << filename << " and made it executable." << std::endl;
        return 0;
    } else {
        std::cerr << "Error: Failed to create or set permissions on " << filename << std::endl;
        return 1;
    }
}