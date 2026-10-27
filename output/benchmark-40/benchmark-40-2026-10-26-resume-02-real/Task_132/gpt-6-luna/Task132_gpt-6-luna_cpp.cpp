#include <algorithm>
#include <chrono>
#include <ctime>
#include <deque>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

enum class EventType {
    AuthenticationFailure,
    AuthenticationSuccess,
    AccessDenied,
    PrivilegeChange,
    SuspiciousNetworkActivity,
    IntegrityViolation
};

struct SecurityEvent {
    EventType type;
    std::string principal;
    std::string source;
    std::string details;
    std::chrono::system_clock::time_point timestamp =
        std::chrono::system_clock::now();
};

struct Alert {
    std::string rule;
    std::string message;
    std::chrono::system_clock::time_point timestamp =
        std::chrono::system_clock::now();
};

std::string eventTypeName(EventType type) {
    switch (type) {
        case EventType::AuthenticationFailure: return "AUTH_FAILURE";
        case EventType::AuthenticationSuccess: return "AUTH_SUCCESS";
        case EventType::AccessDenied: return "ACCESS_DENIED";
        case EventType::PrivilegeChange: return "PRIVILEGE_CHANGE";
        case EventType::SuspiciousNetworkActivity: return "SUSPICIOUS_NETWORK";
        case EventType::IntegrityViolation: return "INTEGRITY_VIOLATION";
    }
    return "UNKNOWN";
}

std::string jsonEscape(const std::string& value) {
    std::ostringstream out;
    for (unsigned char c : value) {
        switch (c) {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\b': out << "\\b"; break;
            case '\f': out << "\\f"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:
                if (c < 0x20) {
                    out << "\\u" << std::hex << std::setw(4)
                        << std::setfill('0') << static_cast<int>(c)
                        << std::dec << std::setfill(' ');
                } else {
                    out << static_cast<char>(c);
                }
        }
    }
    return out.str();
}

std::string formatTimestamp(
    const std::chrono::system_clock::time_point& timestamp) {
    const auto milliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            timestamp.time_since_epoch()) % 1000;
    const std::time_t time = std::chrono::system_clock::to_time_t(timestamp);
    std::tm utc{};
#ifdef _WIN32
    gmtime_s(&utc, &time);
#else
    gmtime_r(&time, &utc);
#endif
    std::ostringstream out;
    out << std::put_time(&utc, "%Y-%m-%dT%H:%M:%S")
        << '.' << std::setw(3) << std::setfill('0') << milliseconds.count()
        << 'Z';
    return out.str();
}

class EventLogger {
public:
    explicit EventLogger(const std::string& path) : file_(path, std::ios::app) {}

    bool isOpen() const {
        return file_.is_open();
    }

    void logEvent(const SecurityEvent& event) {
        std::lock_guard<std::mutex> lock(mutex_);
        file_ << "{\"record\":\"event\",\"timestamp\":\""
              << jsonEscape(formatTimestamp(event.timestamp))
              << "\",\"type\":\"" << eventTypeName(event.type)
              << "\",\"principal\":\"" << jsonEscape(event.principal)
              << "\",\"source\":\"" << jsonEscape(event.source)
              << "\",\"details\":\"" << jsonEscape(event.details)
              << "\"}\n";
        file_.flush();
    }

    void logAlert(const Alert& alert) {
        std::lock_guard<std::mutex> lock(mutex_);
        file_ << "{\"record\":\"alert\",\"timestamp\":\""
              << jsonEscape(formatTimestamp(alert.timestamp))
              << "\",\"rule\":\"" << jsonEscape(alert.rule)
              << "\",\"message\":\"" << jsonEscape(alert.message)
              << "\"}\n";
        file_.flush();
    }

private:
    std::ofstream file_;
    std::mutex mutex_;
};

class SecurityMonitor {
public:
    SecurityMonitor(EventLogger& logger,
                    std::size_t threshold = 5,
                    std::chrono::seconds window = std::chrono::minutes(5))
        : logger_(logger), threshold_(threshold), window_(window) {}

    std::vector<Alert> process(const SecurityEvent& event) {
        logger_.logEvent(event);
        std::vector<Alert> alerts;

        switch (event.type) {
            case EventType::AuthenticationFailure:
                checkRepeatedEvent(
                    "authentication_failures", event.source, event,
                    "Repeated authentication failures from " + event.source,
                    alerts);
                break;
            case EventType::AccessDenied:
                checkRepeatedEvent(
                    "access_denials", event.source, event,
                    "Repeated access denials from " + event.source,
                    alerts);
                break;
            case EventType::PrivilegeChange:
                alerts.push_back(makeAlert(
                    "privilege_change",
                    "Privilege change for " + event.principal +
                        " from " + event.source,
                    event.timestamp));
                break;
            case EventType::SuspiciousNetworkActivity:
                alerts.push_back(makeAlert(
                    "suspicious_network_activity",
                    "Suspicious network activity from " + event.source +
                        ": " + event.details,
                    event.timestamp));
                break;
            case EventType::IntegrityViolation:
                alerts.push_back(makeAlert(
                    "integrity_violation",
                    "Integrity violation involving " + event.details,
                    event.timestamp));
                break;
            case EventType::AuthenticationSuccess:
                break;
        }

        for (const auto& alert : alerts) {
            logger_.logAlert(alert);
        }
        return alerts;
    }

private:
    struct WindowState {
        std::deque<std::chrono::system_clock::time_point> timestamps;
        bool alerted = false;
    };

    static Alert makeAlert(
        const std::string& rule,
        const std::string& message,
        const std::chrono::system_clock::time_point& timestamp) {
        return Alert{rule, message, timestamp};
    }

    void checkRepeatedEvent(
        const std::string& rule,
        const std::string& source,
        const SecurityEvent& event,
        const std::string& message,
        std::vector<Alert>& alerts) {
        const std::string key = rule + "\n" + source;
        auto& state = windows_[key];
        const auto cutoff = event.timestamp - window_;

        while (!state.timestamps.empty() &&
               state.timestamps.front() < cutoff) {
            state.timestamps.pop_front();
        }

        if (state.timestamps.size() < threshold_) {
            state.alerted = false;
        }
        state.timestamps.push_back(event.timestamp);

        if (state.timestamps.size() >= threshold_ && !state.alerted) {
            state.alerted = true;
            alerts.push_back(makeAlert(rule, message, event.timestamp));
        }
    }

    EventLogger& logger_;
    std::size_t threshold_;
    std::chrono::seconds window_;
    std::map<std::string, WindowState> windows_;
};

std::string trim(const std::string& value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

bool parseEventType(const std::string& name, EventType& type) {
    if (name == "AUTH_FAILURE") {
        type = EventType::AuthenticationFailure;
    } else if (name == "AUTH_SUCCESS") {
        type = EventType::AuthenticationSuccess;
    } else if (name == "ACCESS_DENIED") {
        type = EventType::AccessDenied;
    } else if (name == "PRIVILEGE_CHANGE") {
        type = EventType::PrivilegeChange;
    } else if (name == "SUSPICIOUS_NETWORK") {
        type = EventType::SuspiciousNetworkActivity;
    } else if (name == "INTEGRITY_VIOLATION") {
        type = EventType::IntegrityViolation;
    } else {
        return false;
    }
    return true;
}

bool parseEventLine(const std::string& line, SecurityEvent& event) {
    const auto first = line.find('|');
    const auto second = first == std::string::npos
                            ? std::string::npos
                            : line.find('|', first + 1);
    const auto third = second == std::string::npos
                           ? std::string::npos
                           : line.find('|', second + 1);

    if (first == std::string::npos || second == std::string::npos ||
        third == std::string::npos) {
        return false;
    }

    const std::string typeName = trim(line.substr(0, first));
    if (!parseEventType(typeName, event.type)) {
        return false;
    }

    event.principal = trim(line.substr(first + 1, second - first - 1));
    event.source = trim(line.substr(second + 1, third - second - 1));
    event.details = trim(line.substr(third + 1));
    event.timestamp = std::chrono::system_clock::now();
    return true;
}

int main(int argc, char* argv[]) {
    const std::string logPath =
        argc > 1 ? argv[1] : "security_events.jsonl";
    EventLogger logger(logPath);

    if (!logger.isOpen()) {
        std::cerr << "Unable to open log file: " << logPath << '\n';
        return 1;
    }

    SecurityMonitor monitor(logger);
    std::string line;

    while (std::getline(std::cin, line)) {
        const std::string command = trim(line);
        if (command.empty()) {
            continue;
        }
        if (command == "QUIT") {
            break;
        }
        if (command == "HELP") {
            std::cout
                << "Enter TYPE|principal|source|details. Types: "
                << "AUTH_FAILURE, AUTH_SUCCESS, ACCESS_DENIED, "
                << "PRIVILEGE_CHANGE, SUSPICIOUS_NETWORK, "
                << "INTEGRITY_VIOLATION. Enter QUIT to exit.\n";
            continue;
        }

        SecurityEvent event;
        if (!parseEventLine(line, event)) {
            std::cerr << "Invalid event. Use TYPE|principal|source|details "
                         "or enter HELP.\n";
            continue;
        }

        for (const auto& alert : monitor.process(event)) {
            std::cout << "{\"record\":\"alert\",\"timestamp\":\""
                      << jsonEscape(formatTimestamp(alert.timestamp))
                      << "\",\"rule\":\"" << jsonEscape(alert.rule)
                      << "\",\"message\":\""
                      << jsonEscape(alert.message) << "\"}\n";
        }
    }

    return 0;
}