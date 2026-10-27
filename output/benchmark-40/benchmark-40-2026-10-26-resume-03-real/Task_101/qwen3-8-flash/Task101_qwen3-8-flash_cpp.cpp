#include <fstream>
#include <string>
#include <sys/stat.h>

int createExecutableScript(const std::string& filename, const std::string& content) {
    std::ofstream out(filename);
    if (!out) {
        return 1;
    }

    out << content;
    out.close();

    if (chmod(filename.c_str(), 0755) != 0) {
        return 2;
    }

    return 0;
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    return createExecutableScript("script.sh", "#!/bin/sh\necho \"Hello from script.sh\"\n");
}