#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <deque>
#include <ctime>
#include <iomanip>
#include <algorithm>
#include <mutex>

enum class Severity { INFO = 0, LOW, MEDIUM, HIGH, CRITICAL };

static std::string severityToString(Severity s) {
    switch (s) {
        case Severity::INFO: return "INFO";
        case Severity::LOW: return "LOW";
        case Severity::MEDIUM: return "MEDIUM";
        case Severity::HIGH: return "HIGH";
        case Severity::CRITICAL: return "CRITICAL";
    }
    return "UNKNOWN";
}

static bool parseSeverity(const std::string& s, Severity& out) {
    std::string u = s;
    std::transform(u.begin(), u.end(), u.begin(), ::toupper);
    if (u == "INFO") out = Severity::INFO;
    else if (u == "LOW") out = Severity::LOW;
    else if (u == "MEDIUM") out = Severity::MEDIUM;
    else if (u == "HIGH") out = Severity::HIGH;
    else if (u == "CRITICAL") out = Severity::CRITICAL;
    else return false;
    return true;
}

struct SecurityEvent {
    std::time_t timestamp;
    std::string source;
    std::string type;
    std::string user;
    std::string ip;
    Severity severity;
    std::string description;
};

static std::string formatTime(std::time_t t) {
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

static std::string formatEvent(const SecurityEvent& e) {
    std::ostringstream oss;
    oss << "[" << formatTime(e.timestamp) << "] "
        << "[" << severityToString(e.severity) << "] "
        << e.source << " | " << e.type
        << " | user=" << (e.user.empty() ? "-" : e.user)
        << " | ip=" << (e.ip.empty() ? "-" : e.ip)
        << " | " << e.description;
    return oss.str();
}

class SecurityLogger {
public:
    explicit SecurityLogger(const std::string& logPath) : path(logPath) {}

    bool log(const SecurityEvent& e) {
        std::lock_guard<std::mutex> lock(mtx);
        std::ofstream out(path, std::ios::app);
        if (!out) return false;
        out << formatEvent(e) << "\n";
        return static_cast<bool>(out);
    }

    std::vector<SecurityEvent> load() {
        std::lock_guard<std::mutex> lock(mtx);
        std::vector<SecurityEvent> events;
        std::ifstream in(path);
        std::string line;
        while (std::getline(in, line)) {
            SecurityEvent e{};
            e.timestamp = 0;
            e.severity = Severity::INFO;
            e.description = line;
            events.push_back(e);
        }
        return events;
    }

private:
    std::string path;
    std::mutex mtx;
};

class SecurityMonitor {
public:
    SecurityMonitor(int failThreshold, int windowSeconds)
        : threshold(failThreshold), window(windowSeconds) {}

    std::vector<std::string> analyze(const std::vector<SecurityEvent>& events) {
        std::vector<std::string> alerts;
        std::map<std::string, std::deque<std::time_t>> failuresByIp;
        std::map<std::string, int> criticalCountByUser;

        for (const auto& e : events) {
            if (e.severity >= Severity::CRITICAL) {
                criticalCountByUser[e.user.empty() ? "unknown" : e.user]++;
                alerts.push_back("ALERT: critical event from " + e.source +
                                 " (" + e.type + ") user=" + e.user);
            }
            if (e.type == "LOGIN_FAILURE" && !e.ip.empty()) {
                auto& dq = failuresByIp[e.ip];
                dq.push_back(e.timestamp);
                while (!dq.empty() && e.timestamp - dq.front() > window) {
                    dq.pop_front();
                }
                if (static_cast<int>(dq.size()) >= threshold) {
                    alerts.push_back("ALERT: brute force suspected from ip=" + e.ip +
                                     " (" + std::to_string(dq.size()) +
                                     " failures within " + std::to_string(window) + "s)");
                    dq.clear();
                }
            }
        }
        for (const auto& kv : criticalCountByUser) {
            if (kv.second >= 3) {
                alerts.push_back("ALERT: repeated critical events for user=" + kv.first +
                                 " count=" + std::to_string(kv.second));
            }
        }
        return alerts;
    }

private:
    int threshold;
    int window;
};

static SecurityEvent makeEvent(std::time_t ts, const std::string& source,
                               const std::string& type, const std::string& user,
                               const std::string& ip, Severity sev,
                               const std::string& desc) {
    SecurityEvent e;
    e.timestamp = ts;
    e.source = source;
    e.type = type;
    e.user = user;
    e.ip = ip;
    e.severity = sev;
    e.description = desc;
    return e;
}

static std::vector<SecurityEvent> simulateEvents(std::time_t base) {
    return {
        makeEvent(base + 0,  "auth",     "LOGIN_SUCCESS",  "alice", "10.0.0.5",    Severity::INFO,     "Successful login"),
        makeEvent(base + 10, "auth",     "LOGIN_FAILURE",  "bob",   "192.168.1.9", Severity::LOW,      "Bad password"),
        makeEvent(base + 12, "auth",     "LOGIN_FAILURE",  "bob",   "192.168.1.9", Severity::LOW,      "Bad password"),
        makeEvent(base + 15, "auth",     "LOGIN_FAILURE",  "root",  "192.168.1.9", Severity::MEDIUM,   "Bad password"),
        makeEvent(base + 18, "auth",     "LOGIN_FAILURE",  "admin", "192.168.1.9", Severity::MEDIUM,   "Bad password"),
        makeEvent(base + 30, "firewall", "PORT_SCAN",      "",      "203.0.113.7", Severity::HIGH,     "Sequential port scan detected"),
        makeEvent(base + 40, "fim",      "FILE_MODIFIED",  "eve",   "",            Severity::CRITICAL, "/etc/passwd modified"),
        makeEvent(base + 45, "fim",      "FILE_MODIFIED",  "eve",   "",            Severity::CRITICAL, "/etc/shadow modified"),
        makeEvent(base + 50, "fim",      "FILE_MODIFIED",  "eve",   "",            Severity::CRITICAL, "/root/.ssh/authorized_keys modified"),
        makeEvent(base + 60, "ids",      "MALWARE_HASH",   "",      "10.0.0.8",    Severity::HIGH,     "Known malicious hash matched")
    };
}

int main() {
    const std::string logPath = "security_events.log";
    SecurityLogger logger(logPath);
    SecurityMonitor monitor(4, 60);

    std::time_t base = std::time(nullptr);
    std::vector<SecurityEvent> events = simulateEvents(base);

    int written = 0;
    for (const auto& e : events) {
        if (logger.log(e)) ++written;
    }

    std::cout << "Logged " << written << " of " << events.size()
              << " events to " << logPath << "\n\n";

    std::cout << "--- Alerts ---\n";
    auto alerts = monitor.analyze(events);
    if (alerts.empty()) {
        std::cout << "No alerts.\n";
    } else {
        for (const auto& a : alerts) std::cout << a << "\n";
    }

    std::cout << "\n--- Log Contents ---\n";
    for (const auto& e : events) {
        std::cout << formatEvent(e) << "\n";
    }

    return 0;
}