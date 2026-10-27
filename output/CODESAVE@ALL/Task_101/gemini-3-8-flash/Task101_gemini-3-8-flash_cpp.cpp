#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

bool createExecutableScript(const std::filesystem::path& filePath, const std::string& content) {
    std::ofstream outFile(filePath);
    if (!outFile) {
        return false;
    }
    
    outFile << content;
    outFile.close();

    std::error_code ec;
    std::filesystem::permissions(
        filePath,
        std::filesystem::perms::owner_exec | std::filesystem::perms::group_exec | std::filesystem::perms::others_exec,
        std::filesystem::perm_options::add,
        ec
    );

    return !ec;
}

int main(int argc, char* argv[]) {
    const std::filesystem::path scriptPath = "script.sh";
    const std::string scriptContent = "#!/bin/sh\necho \"Script executed successfully.\"\n";

    if (createExecutableScript(scriptPath, scriptContent)) {
        std::cout << "Successfully created and set executable permissions for " << scriptPath << '\n';
        return 0;
    } else {
        std::cerr << "Failed to create or set permissions for " << scriptPath << '\n';
        return 1;
    }
}