#include <arpa/inet.h>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <map>
#include <mutex>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace {

constexpr char kCookieName[] = "__Host-session";
constexpr auto kSessionLifetime = std::chrono::minutes(30);
constexpr std::size_t kMaxHeaderBytes = 16 * 1024;
constexpr std::size_t kMaxBodyBytes = 8 * 1024;

struct Request {
    std::string method;
    std::string target;
    std::map<std::string, std::string> headers;
    std::string body;
};

struct Response {
    int status = 200;
    std::string contentType = "application/json; charset=utf-8";
    std::string body;
    std::vector<std::pair<std::string, std::string>> headers;
};

struct Session {
    std::string csrfToken;
    std::chrono::steady_clock::time_point expiresAt;
};

std::string lower(std::string value) {
    for (char& c : value) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return value;
}

std::string trim(const std::string& value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

bool fillSecureRandom(unsigned char* buffer, std::size_t length) {
    int fd = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        return false;
    }

    std::size_t offset = 0;
    while (offset < length) {
        const ssize_t count = read(fd, buffer + offset, length - offset);
        if (count < 0 && errno == EINTR) {
            continue;
        }
        if (count <= 0) {
            close(fd);
            return false;
        }
        offset += static_cast<std::size_t>(count);
    }

    close(fd);
    return true;
}

std::optional<std::string> randomHex(std::size_t byteCount) {
    std::vector<unsigned char> bytes(byteCount);
    if (!fillSecureRandom(bytes.data(), bytes.size())) {
        return std::nullopt;
    }

    static constexpr char hex[] = "0123456789abcdef";
    std::string result;
    result.reserve(byteCount * 2);
    for (unsigned char byte : bytes) {
        result.push_back(hex[byte >> 4]);
        result.push_back(hex[byte & 0x0f]);
    }
    return result;
}

bool isSessionToken(const std::string& token) {
    if (token.size() != 64) {
        return false;
    }
    for (unsigned char c : token) {
        if (!std::isxdigit(c)) {
            return false;
        }
    }
    return true;
}

bool constantTimeEqual(const std::string& a, const std::string& b) {
    const std::size_t maximum = std::max(a.size(), b.size());
    unsigned char difference = static_cast<unsigned char>(a.size() ^ b.size());
    for (std::size_t i = 0; i < maximum; ++i) {
        const unsigned char left = i < a.size() ? static_cast<unsigned char>(a[i]) : 0;
        const unsigned char right = i < b.size() ? static_cast<unsigned char>(b[i]) : 0;
        difference |= left ^ right;
    }
    return difference == 0;
}

class SessionStore {
public:
    bool create(const std::optional<std::string>& previousId,
                std::string& newId,
                std::string& csrfToken) {
        auto generatedId = randomHex(32);
        auto generatedCsrf = randomHex(32);
        if (!generatedId || !generatedCsrf) {
            return false;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        pruneExpiredLocked();

        while (sessions_.find(*generatedId) != sessions_.end()) {
            generatedId = randomHex(32);
            if (!generatedId) {
                return false;
            }
        }

        if (previousId) {
            sessions_.erase(*previousId);
        }

        sessions_.emplace(*generatedId,
                          Session{*generatedCsrf,
                                  std::chrono::steady_clock::now() + kSessionLifetime});
        newId = *generatedId;
        csrfToken = *generatedCsrf;
        return true;
    }

    std::optional<std::string> touch(const std::string& id) {
        if (!isSessionToken(id)) {
            return std::nullopt;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        const auto now = std::chrono::steady_clock::now();
        auto it = sessions_.find(id);
        if (it == sessions_.end()) {
            return std::nullopt;
        }
        if (it->second.expiresAt <= now) {
            sessions_.erase(it);
            return std::nullopt;
        }

        it->second.expiresAt = now + kSessionLifetime;
        return it->second.csrfToken;
    }

    bool eraseIfCsrfMatches(const std::string& id, const std::string& csrfToken) {
        if (!isSessionToken(id) || !isSessionToken(csrfToken)) {
            return false;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        const auto now = std::chrono::steady_clock::now();
        auto it = sessions_.find(id);
        if (it == sessions_.end() || it->second.expiresAt <= now) {
            if (it != sessions_.end()) {
                sessions_.erase(it);
            }
            return false;
        }
        if (!constantTimeEqual(it->second.csrfToken, csrfToken)) {
            return false;
        }
        sessions_.erase(it);
        return true;
    }

    void erase(const std::string& id) {
        if (!isSessionToken(id)) {
            return;
        }
        std::lock_guard<std::mutex> lock(mutex_);
        sessions_.erase(id);
    }

private:
    void pruneExpiredLocked() {
        const auto now = std::chrono::steady_clock::now();
        for (auto it = sessions_.begin(); it != sessions_.end();) {
            if (it->second.expiresAt <= now) {
                it = sessions_.erase(it);
            } else {