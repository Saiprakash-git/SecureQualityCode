#include <fstream>
#include <sys/stat.h>

int main() {
    {
        std::ofstream script("script.sh");
        if (!script) {
            return 1;
        }
        script << "#!/bin/sh\n";
        if (!script) {
            return 1;
        }
    }

    return chmod("script.sh", 0755) == 0 ? 0 : 1;
}