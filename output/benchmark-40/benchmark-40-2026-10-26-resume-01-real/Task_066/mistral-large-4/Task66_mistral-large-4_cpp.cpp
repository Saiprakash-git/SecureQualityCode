#include <iostream>
#include <string>
#include <cstdio>
#include <array>
#include <sys/wait.h>

struct CommandResult {
    std::string output;
    int exitCode;
    bool success;
};

bool validateCommand(const std::string& command) {
    if (command.empty()) {
        return false;
    }
    if (command.length() > 8192) {
        return false;
    }
    if (command.find('\0') != std::string::npos) {
        return false;
    }
    if (command.find_first_not_of(" \t\n\r") == std::string::npos) {
        return false;
    }
    return true;
}

CommandResult executeCommand(const std::string& command) {
    CommandResult result;
    result.exitCode = -1;
    result.success = false;

    if (!validateCommand(command)) {
        result.output = "Error: Invalid command format or content";
        return result;
    }

    std::string fullCommand = command + " 2>&1";
    std::array<char, 256> buffer;
    std::string output;

    FILE* pipe = popen(fullCommand.c_str(), "r");
    if (!pipe) {
        result.output = "Error: Failed to execute command";
        return result;
    }

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        output += buffer.data();
    }

    int status = pclose(pipe);
    if (status == -1) {
        result.exitCode = -1;
        result.output = output + "\nError: Failed to close pipe";
        return result;
    }

    if (WIFEXITED(status)) {
        result.exitCode = WEXITSTATUS(status);
    } else {
        result.exitCode = -1;
    }

    result.output = output;
    result.success = (result.exitCode == 0);
    return result;
}

std::string readCommand() {
    std::string command;
    std::cout << "Enter bash command: ";
    std::getline(std::cin, command);
    return command;
}

int main() {
    std::string command = readCommand();
    CommandResult result = executeCommand(command);

    std::cout << "Output:\n" << result.output;
    if (!result.output.empty() && result.output.back() != '\n') {
        std::cout << "\n";
    }
    std::cout << "Exit Code: " << result.exitCode << "\n";
    std::cout << "Success: " << (result.success ? "Yes" : "No") << "\n";

    return result.exitCode;
}