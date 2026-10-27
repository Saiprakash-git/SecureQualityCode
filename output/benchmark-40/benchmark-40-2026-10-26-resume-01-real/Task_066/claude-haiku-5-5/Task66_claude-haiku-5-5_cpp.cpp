#include <iostream>
#include <string>
#include <vector>
#include <cstdio>
#include <cctype>
#include <sys/wait.h>

struct ValidationResult {
    bool ok;
    std::string error;
};

struct ExecutionResult {
    bool ok;
    int exitCode;
    std::string output;
    std::string error;
};

static const size_t MAX_COMMAND_LENGTH = 4096;

std::string trim(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) {
        ++start;
    }
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
        --end;
    }
    return s.substr(start, end - start);
}

std::string normalizeForScan(const std::string& s) {
    std::string out;
    bool lastSpace = false;
    for (unsigned char c : s) {
        if (std::isspace(c)) {
            if (!lastSpace) {
                out += ' ';
            }
            lastSpace = true;
        } else {
            out += static_cast<char>(std::tolower(c));
            lastSpace = false;
        }
    }
    return out;
}

ValidationResult validateFormat(const std::string& rawCommand) {
    std::string cmd = trim(rawCommand);

    if (cmd.empty()) {
        return {false, "Command is empty."};
    }
    if (cmd.size() > MAX_COMMAND_LENGTH) {
        return {false, "Command exceeds maximum length of " + std::to_string(MAX_COMMAND_LENGTH) + " characters."};
    }

    for (unsigned char c : cmd) {
        if (c == '\0') {
            return {false, "Command contains NUL characters."};
        }
        if (std::iscntrl(c) && c != '\t') {
            return {false, "Command contains disallowed control characters."};
        }
    }

    char first = cmd[0];
    if (first == ';' || first == '|' || first == '&' || first == '(' || first == ')') {
        return {false, "Command cannot start with a shell control operator."};
    }

    size_t tokenEnd = 0;
    while (tokenEnd < cmd.size() && !std::isspace(static_cast<unsigned char>(cmd[tokenEnd]))) {
        ++tokenEnd;
    }
    std::string program = cmd.substr(0, tokenEnd);

    if (program.empty() || program[0] == '-') {
        return {false, "Command must begin with a valid program name."};
    }
    for (unsigned char c : program) {
        if (!(std::isalnum(c) || c == '_' || c == '.' || c == '/' || c == '-')) {
            return {false, "Program name '" + program + "' contains invalid characters."};
        }
    }

    bool inSingle = false;
    bool inDouble = false;
    for (size_t i = 0; i < cmd.size(); ++i) {
        char c = cmd[i];
        if (inSingle) {
            if (c == '\'') {
                inSingle = false;
            }
            continue;
        }
        if (c == '\\') {
            if (i + 1 >= cmd.size()) {
                return {false, "Command ends with an unterminated backslash."};
            }
            ++i;
            continue;
        }
        if (inDouble) {
            if (c == '"') {
                inDouble = false;
            }
            continue;
        }
        if (c == '\'') {
            inSingle = true;
        } else if (c == '"') {
            inDouble = true;
        }
    }
    if (inSingle) {
        return {false, "Unterminated single quote."};
    }
    if (inDouble) {
        return {false, "Unterminated double quote."};
    }

    char last = cmd[cmd.size() - 1];
    if (last == '|' || last == '&' || last == '>' || last == '<') {
        return {false, "Command ends with an incomplete operator."};
    }

    return {true, ""};
}

ValidationResult validateContent(const std::string& rawCommand) {
    std::string scan = normalizeForScan(trim(rawCommand));

    static const std::vector<std::pair<std::string, std::string>> blocked = {
        {"rm -rf /", "Recursive deletion of root filesystem is not allowed."},
        {"rm -fr /", "Recursive deletion of root filesystem is not allowed."},
        {"rm -rf /*", "Recursive deletion of root filesystem is not allowed."},
        {"mkfs", "Filesystem formatting commands are not allowed."},
        {"dd if=", "Raw disk copy commands are not allowed."},
        {"> /dev/sd", "Writing directly to disk devices is not allowed."},
        {"> /dev/nvme", "Writing directly to disk devices is not allowed."},
        {":(){", "Fork bombs are not allowed."},
        {"shutdown", "System shutdown commands are not allowed."},
        {"reboot", "System reboot commands are not allowed."},
        {"poweroff", "System power-off commands are not allowed."},
        {"halt", "System halt commands are not allowed."},
        {"init 0", "System halt commands are not allowed."},
        {"chmod -r 777 /", "Recursive permission changes on root are not allowed."},
        {"chown -r", "Recursive ownership changes are not allowed."},
        {"sudo", "Privilege escalation via sudo is not allowed."},
        {"su -", "Switching to root is not allowed."},
        {"passwd", "Password modification commands are not allowed."},
        {"/etc/shadow", "Access to shadow password file is not allowed."},
        {"nc -e", "Reverse shell patterns are not allowed."},
        {"/dev/tcp/", "Network redirection via /dev/tcp is not allowed."},
        {"curl", "Network download commands are not allowed."},
        {"wget", "Network download commands are not allowed."}
    };

    for (const auto& entry : blocked) {
        if (scan.find(entry.first) != std::string::npos) {
            return {false, entry.second + " (matched '" + entry.first + "')"};
        }
    }

    return {true, ""};
}

std::string shellSingleQuote(const std::string& s) {
    std::string out = "'";
    for (char c : s) {
        if (c == '\'') {
            out += "'\\''";
        } else {
            out += c;
        }
    }
    out += "'";
    return out;
}

ExecutionResult executeCommand(const std::string& command) {
    ExecutionResult result{false, -1, "", ""};

    std::string wrapped = "bash -c " + shellSingleQuote(command) + " 2>&1";
    FILE* pipe = popen(wrapped.c_str(), "r");
    if (!pipe) {
        result.error = "Failed to start process.";
        return result;
    }

    std::array<char, 4096> buffer{};
    while (std::fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        result.output += buffer.data();
    }

    int status = pclose(pipe);
    if (status == -1) {
        result.error = "Failed to obtain process status.";
        return result;
    }

    if (WIFEXITED(status)) {
        result.exitCode = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        result.exitCode = 128 + WTERMSIG(status);
    }

    result.ok = true;
    return result;
}

int main() {
    std::cout << "Enter a bash command: " << std::flush;

    std::string input;
    if (!std::getline(std::cin, input)) {
        std::cerr << "Error: no input received." << std::endl;
        return 1;
    }

    ValidationResult format = validateFormat(input);
    if (!format.ok) {
        std::cerr << "Format validation failed: " << format.error << std::endl;
        return 2;
    }

    ValidationResult content = validateContent(input);
    if (!content.ok) {