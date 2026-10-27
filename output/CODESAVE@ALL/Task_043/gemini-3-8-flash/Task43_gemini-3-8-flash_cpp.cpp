#include <iostream>
#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <sstream>
#include <vector>
#include <iomanip>
#include <thread>
#include <optional>
#include <cstring>
#include <csignal>
#include <fstream>
#include <random>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

struct Session {
    std::string id;
    std::string username;
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point lastAccessed;
    std::chrono::system_clock::time_point expiresAt;
};

struct HttpRequest {
    std::string method;
    std::string path;
    std::unordered_map<std::string, std::string> headers;
    std::unordered_map<std::string, std::string> cookies;
    std::string body;
};

struct HttpResponse {
    int statusCode;
    std::string statusText;
    std::unordered_map<std::string, std::string> headers;
    std::string body;

    std::string toString() const {
        std::ostringstream stream;
        stream << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n";
        for (const auto& [key, value] : headers) {
            stream << key << ": " << value << "\r\n";
        }
        stream << "Content-Length: " << body.length() << "\r\n";
        stream << "Connection: close\r\n\r\n";
        stream << body;
        return stream.str();
    }
};

class SecureTokenGenerator {
public:
    static std::string generateToken(size_t byteCount = 32) {
        std::vector<unsigned char> buffer(byteCount);
        std::ifstream urandom("/dev/urandom", std::ios::in | std::ios::binary);
        if (urandom.is_open()) {
            urandom.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(byteCount));
            urandom.close();
        } else {
            std::random_device rd;
            for (size_t i = 0; i < byteCount; ++i) {
                buffer[i] = static_cast<unsigned char>(rd() & 0xFF);
            }
        }
        std::ostringstream oss;
        for (unsigned char b : buffer) {
            oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
        }
        return oss.str();
    }
};

class SessionStore {
private:
    std::unordered_map<std::string, Session> sessions_;
    mutable std::mutex mutex_;
    std::chrono::seconds idleTimeout_;
    std::chrono::seconds absoluteTimeout_;

public:
    SessionStore(std::chrono::seconds idleTimeout = std::chrono::seconds(900),
                 std::chrono::seconds absoluteTimeout = std::chrono::seconds(3600))
        : idleTimeout_(idleTimeout), absoluteTimeout_(absoluteTimeout) {}

    std::string createSession(const std::string& username) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::string sessionId = SecureTokenGenerator::generateToken(32);
        auto now = std::chrono::system_clock::now();

        Session session;
        session.id = sessionId;
        session.username = username;
        session.createdAt = now;
        session.lastAccessed = now;
        session.expiresAt = now + absoluteTimeout_;

        sessions_[sessionId] = session;
        return sessionId;
    }

    std::optional<Session> getAndValidateSession(const std::string& sessionId) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = sessions_.find(sessionId);
        if (it == sessions_.end()) {
            return std::nullopt;
        }

        auto now = std::chrono::system_clock::now();

        if (now > it->second.expiresAt || (now - it->second.lastAccessed) > idleTimeout_) {
            sessions_.erase(it);
            return std::nullopt;
        }

        it->second.lastAccessed = now;
        return it->second;
    }

    bool terminateSession(const std::string& sessionId) {
        std::lock_guard<std::mutex> lock(mutex_);
        return sessions_.erase(sessionId) > 0;
    }

    std::string rotateSession(const std::string& oldSessionId) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = sessions_.find(oldSessionId);
        if (it == sessions_.end()) {
            return "";
        }

        Session oldSession = it->second;
        sessions_.erase(it);

        std::string newSessionId = SecureTokenGenerator::generateToken(32);
        auto now = std::chrono::system_clock::now();

        Session newSession;
        newSession.id = newSessionId;
        newSession.username = oldSession.username;
        newSession.createdAt = oldSession.createdAt;
        newSession.lastAccessed = now;
        newSession.expiresAt = oldSession.expiresAt;

        sessions_[newSessionId] = newSession;
        return newSessionId;
    }

    void purgeExpiredSessions() {
        std::lock_guard<std::mutex> lock(mutex_);
        auto now = std::chrono::system_clock::now();
        for (auto it = sessions_.begin(); it != sessions_.end();) {
            if (now > it->second.expiresAt || (now - it->second.lastAccessed) > idleTimeout_) {
                it = sessions_.erase(it);
            } else {
                ++it;
            }
        }
    }
};

class HttpParser {
public:
    static std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }

    static std::unordered_map<std::string, std::string> parseUrlEncoded(const std::string& query) {
        std::unordered_map<std::string, std::string> result;
        std::istringstream stream(query);
        std::string pair;
        while (std::getline(stream, pair, '&')) {
            size_t eqPos = pair.find('=');
            if (eqPos != std::string::npos) {
                std::string key = pair.substr(0, eqPos);
                std::string val = pair.substr(eqPos + 1);
                result[key] = val;
            }
        }
        return result;
    }

    static std::unordered_map<std::string, std::string> parseCookies(const std::string& cookieHeader) {
        std::unordered_map<std::string, std::string> cookies;
        std::istringstream stream(cookieHeader);
        std::string item;
        while (std::getline(stream, item, ';')) {
            item = trim(item);
            size_t eqPos = item.find('=');
            if (eqPos != std::string::npos) {
                std::string key = trim(item.substr(0, eqPos));
                std::string val = trim(item.substr(eqPos + 1));
                cookies[key] = val;
            }
        }
        return cookies;
    }

    static HttpRequest parse(const std::string& raw) {
        HttpRequest req;
        std::istringstream stream(raw);
        std::string line;

        if (std::getline(stream, line)) {
            line = trim(line);
            size_t firstSpace = line.find(' ');
            size_t secondSpace = line.find(' ', firstSpace + 1);
            if (firstSpace != std::string::npos && secondSpace != std::string::npos) {
                req.method = line.substr(0, firstSpace);
                req.path = line.substr(firstSpace + 1, secondSpace - firstSpace - 1);
            }
        }

        while (std::getline(stream, line)) {
            line = trim(line);
            if (line.empty()) {
                break;
            }
            size_t colon = line.find(':');
            if (colon != std::string::npos) {
                std::string headerName = trim(line.substr(0, colon));
                std::string headerVal = trim(line.substr(colon + 1));
                req.headers[headerName] = headerVal;
                if (headerName == "Cookie") {
                    req.cookies = parseCookies(headerVal);
                }
            }
        }

        std::ostringstream bodyStream;
        while (std::getline(stream, line)) {
            bodyStream << line << "\n";
        }
        req.body = bodyStream.str();
        if (!req.body.empty() && req.body.back() == '\n') {
            req.body.pop_back();
        }

        return req;
    }
};

class WebApp {
private:
    SessionStore sessionStore_;
    int serverFd_;
    int port_;
    bool running_;

    std::string extractCookieSessionId(const HttpRequest& req) const {
        auto it = req.cookies.find("SESSIONID");
        if (it