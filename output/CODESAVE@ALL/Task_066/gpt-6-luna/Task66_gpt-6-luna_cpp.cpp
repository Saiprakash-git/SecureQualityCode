#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {

bool isForbidden(char c) {
    switch (c) {
        case '$':
        case '`':
        case ';':
        case '|':
        case '&':
        case '<':
        case '>':
        case '\\':
        case '(':
        case ')':
        case '*':
        case '?':
        case '[':
        case ']':
        case '{':
        case '}':
        case '!':
            return true;
        default:
            return c == '\0' || c == '\n' || c == '\r';
    }
}

bool tokenize(const std::string& input, std::vector<std::string>& tokens) {
    if (input.empty() || input.size() > 4096) {
        return false;
    }

    std::string token;
    char quote = '\0';
    bool tokenStarted = false;

    for (char c : input) {
        if (isForbidden(c)) {
            return false;
        }

        if (quote != '\0') {
            if (c == quote) {
                quote = '\0';
            } else {
                token.push_back(c);
            }
            tokenStarted = true;
        } else if (c == '\'' || c == '"') {
            quote = c;
            tokenStarted = true;
        } else if (c == ' ' || c == '\t') {
            if (tokenStarted) {
                tokens.push_back(token);
                token.clear();
                tokenStarted = false;
                if (tokens.size() > 64) {
                    return false;
                }
            }
        } else {
            token.push_back(c);
            tokenStarted = true;
        }

        if (token.size() > 1024) {
            return false;
        }
    }

    if (quote != '\0') {
        return false;
    }

    if (tokenStarted) {
        tokens.push_back(token);
    }

    return !tokens.empty() && tokens.size() <= 64;
}

bool selectExecutable(const std::vector<std::string>& tokens,
                      std::string& executable) {
    const std::string& command = tokens.front();

    if (command == "echo") {
        executable = "/bin/echo";
        return true;
    }
    if (command == "pwd" && tokens.size() == 1) {
        executable = "/bin/pwd";
        return true;
    }
    if (command == "date" && tokens.size() == 1) {
        executable = "/bin/date";
        return true;
    }
    if (command == "whoami" && tokens.size() == 1) {
        executable = "/usr/bin/whoami";
        return true;
    }
    if (command == "uname" && tokens.size() == 1) {
        executable = "/bin/uname";
        return true;
    }

    return false;
}

bool execute(const std::vector<std::string>& tokens,
             const std::string& executable,
             std::string& output) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        return false;
    }

    pid_t pid = fork();
    if (pid == -1) {
        close(pipefd[0]);
        close(pipefd[1]);
        return false;
    }

    if (pid == 0) {
        close(pipefd[0]);

        if (dup2(pipefd[1], STDOUT_FILENO) == -1 ||
            dup2(pipefd[1], STDERR_FILENO) == -1) {
            _exit(126);
        }
        close(pipefd[1]);

        std::vector<char*> argv;
        argv.reserve(tokens.size() + 1);
        for (const std::string& token : tokens) {
            argv.push_back(const_cast<char*>(token.c_str()));
        }
        argv.push_back(nullptr);

        execv(executable.c_str(), argv.data());
        _exit(errno == ENOENT ? 127 : 126);
    }

    close(pipefd[1]);
    char buffer[4096];
    for (;;) {
        ssize_t count = read(pipefd[0], buffer, sizeof(buffer));
        if (count > 0) {
            output.append(buffer, static_cast<std::size_t>(count));
        } else if (count == 0) {
            break;
        } else if (errno != EINTR) {
            close(pipefd[0]);
            int status;
            while (waitpid(pid, &status, 0) == -1 && errno == EINTR) {}
            return false;
        }
    }
    close(pipefd[0]);

    int status;
    while (waitpid(pid, &status, 0) == -1) {
        if (errno != EINTR) {
            return false;
        }
    }

    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

}  // namespace

int main() {
    std::string input;
    if (!std::getline(std::cin, input)) {
        std::cerr << "Invalid input\n";
        return 1;
    }

    std::vector<std::string> tokens;
    if (!tokenize(input, tokens)) {
        std::cerr << "Invalid command format\n";
        return 1;
    }

    std::string executable;
    if (!selectExecutable(tokens, executable)) {
        std::cerr << "Command not allowed\n";
        return 1;
    }

    std::string output;
    if (!execute(tokens, executable, output)) {
        std::cout << output;
        std::cerr << "Command execution failed\n";
        return 1;
    }

    std::cout << output;
    return 0;
}