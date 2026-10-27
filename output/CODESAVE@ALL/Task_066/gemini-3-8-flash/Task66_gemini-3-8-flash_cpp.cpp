#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <array>
#include <memory>
#include <cctype>
#include <algorithm>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

struct ValidationResult {
    bool is_valid;
    std::string message;
};

struct ExecutionResult {
    int exit_code;
    std::string stdout_output;
    std::string stderr_output;
};

bool has_balanced_quotes(const std::string& cmd) {
    bool in_single_quote = false;
    bool in_double_quote = false;
    bool escaped = false;

    for (size_t i = 0; i < cmd.length(); ++i) {
        char c = cmd[i];

        if (escaped) {
            escaped = false;
            continue;
        }

        if (c == '\\' && !in_single_quote) {
            escaped = true;
            continue;
        }

        if (c == '\'' && !in_double_quote) {
            in_single_quote = !in_single_quote;
        } else if (c == '"' && !in_single_quote) {
            in_double_quote = !in_double_quote;
        }
    }

    return !in_single_quote && !in_double_quote && !escaped;
}

bool contains_disallowed_patterns(const std::string& cmd, std::string& matched_pattern) {
    static const std::vector<std::string> blacklist = {
        ":(){ :|:& };:",
        "rm -rf /",
        "rm -rf /*",
        "> /dev/sda",
        "mkfs",
        "dd if=/dev/zero",
        "dd if=/dev/random",
        ":(){:|:&};:"
    };

    std::string stripped;
    for (char c : cmd) {
        if (!std::isspace(static_cast<unsigned char>(c))) {
            stripped += std::tolower(static_cast<unsigned char>(c));
        }
    }

    for (const auto& pattern : blacklist) {
        std::string stripped_pattern;
        for (char c : pattern) {
            if (!std::isspace(static_cast<unsigned char>(c))) {
                stripped_pattern += std::tolower(static_cast<unsigned char>(c));
            }
        }
        if (stripped.find(stripped_pattern) != std::string::npos) {
            matched_pattern = pattern;
            return true;
        }
    }

    return false;
}

bool check_bash_syntax(const std::string& cmd, std::string& syntax_err) {
    int pipe_err[2];
    if (pipe(pipe_err) == -1) {
        syntax_err = "Internal system error: failed to create pipe.";
        return false;
    }

    pid_t pid = fork();
    if (pid == -1) {
        close(pipe_err[0]);
        close(pipe_err[1]);
        syntax_err = "Internal system error: failed to fork process.";
        return false;
    }

    if (pid == 0) {
        close(pipe_err[0]);
        dup2(pipe_err[1], STDERR_FILENO);
        close(pipe_err[1]);

        int dev_null = open("/dev/null", O_WRONLY);
        if (dev_null != -1) {
            dup2(dev_null, STDOUT_FILENO);
            close(dev_null);
        }

        execl("/bin/bash", "bash", "-n", "-c", cmd.c_str(), static_cast<char*>(nullptr));
        _exit(127);
    }

    close(pipe_err[1]);

    std::array<char, 256> buffer;
    ssize_t bytes_read;
    while ((bytes_read = read(pipe_err[0], buffer.data(), buffer.size())) > 0) {
        syntax_err.append(buffer.data(), bytes_read);
    }
    close(pipe_err[0]);

    int status = 0;
    waitpid(pid, &status, 0);

    return (WIFEXITED(status) && WEXITSTATUS(status) == 0);
}

ValidationResult validate_command(const std::string& cmd) {
    if (cmd.empty() || cmd.find_first_not_of(" \t\n\r") == std::string::npos) {
        return {false, "Validation failed: Command cannot be empty."};
    }

    if (cmd.length() > 4096) {
        return {false, "Validation failed: Command exceeds maximum length of 4096 characters."};
    }

    for (unsigned char c : cmd) {
        if (!std::isprint(c) && c != '\t' && c != '\n') {
            return {false, "Validation failed: Command contains invalid control characters."};
        }
    }

    if (!has_balanced_quotes(cmd)) {
        return {false, "Validation failed: Command contains unclosed quotes or dangling escapes."};
    }

    std::string matched_pattern;
    if (contains_disallowed_patterns(cmd, matched_pattern)) {
        return {false, "Validation failed: Command contains prohibited destructive pattern: " + matched_pattern};
    }

    std::string syntax_error;
    if (!check_bash_syntax(cmd, syntax_error)) {
        return {false, "Validation failed (Bash Syntax Error):\n" + syntax_error};
    }

    return {true, "Command is valid."};
}

ExecutionResult execute_command(const std::string& cmd) {
    int pipe_out[2];
    int pipe_err[2];

    if (pipe(pipe_out) == -1 || pipe(pipe_err) == -1) {
        return {-1, "", "Failed to allocate pipes for execution."};
    }

    pid_t pid = fork();
    if (pid == -1) {
        close(pipe_out[0]);
        close(pipe_out[1]);
        close(pipe_err[0]);
        close(pipe_err[1]);
        return {-1, "", "Failed to fork process."};
    }

    if (pid == 0) {
        close(pipe_out[0]);
        dup2(pipe_out[1], STDOUT_FILENO);
        close(pipe_out[1]);

        close(pipe_err[0]);
        dup2(pipe_err[1], STDERR_FILENO);
        close(pipe_err[1]);

        execl("/bin/bash", "bash", "-c", cmd.c_str(), static_cast<char*>(nullptr));
        _exit(127);
    }

    close(pipe_out[1]);
    close(pipe_err[1]);

    ExecutionResult result;
    std::array<char, 512> buffer;
    ssize_t bytes_read;

    while ((bytes_read = read(pipe_out[0], buffer.data(), buffer.size())) > 0) {
        result.stdout_output.append(buffer.data(), bytes_read);
    }
    close(pipe_out[0]);

    while ((bytes_read = read(pipe_err[0], buffer.data(), buffer.size())) > 0) {
        result.stderr_output.append(buffer.data(), bytes_read);
    }
    close(pipe_err[0]);

    int status = 0;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status)) {
        result.exit_code = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        result.exit_code = 128 + WTERMSIG(status);
    } else {
        result.exit_code = -1;
    }

    return result;
}

std::string read_input_command() {
    std::string line;
    std::cout << "bash> ";
    if (!std::getline(std::cin, line)) {
        return "";
    }
    return line;
}

void display_result(const ExecutionResult& result) {
    std::cout << "\n--- Execution Output ---" << std::endl;
    std::cout << "Exit Code: " << result.exit_code << std::endl;
    if (!result.stdout_output.empty()) {
        std::cout << "[STDOUT]:\n" << result.stdout_output;
        if (result.stdout_output.back() != '\n') {
            std::cout << std::endl;
        }
    }
    if (!result.stderr_output.empty()) {
        std::cout << "[STDERR]:\n" << result.stderr_output;
        if (result.stderr_output.back() != '\n') {
            std::cout << std::endl;
        }
    }
    std::cout << "------------------------" << std::endl;
}

int main() {
    std::string cmd = read_input_command();
    if (cmd.empty() && std::cin.eof()) {
        return 0;
    }

    ValidationResult validation = validate_command(cmd);
    if (!validation.is_valid) {
        std::cerr << validation.message << std::endl;
        return 1;
    }

    ExecutionResult result = execute_command(cmd);
    display_result(result);

    return result.exit_code;
}