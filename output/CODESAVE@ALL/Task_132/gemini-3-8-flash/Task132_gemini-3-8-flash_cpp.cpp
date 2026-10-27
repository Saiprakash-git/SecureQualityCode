#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <algorithm>

enum class Severity {
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

enum class EventType {
    LOGIN_SUCCESS,
    LOGIN_FAILURE,
    ACCESS_DENIED,
    PRIVILEGE_ESCALATION,
    FILE_INTEGRITY_VIOLATION,
    SUSPICIOUS_NETWORK_ACTIVITY
};

struct SecurityEvent {
    std::chrono::system_clock::time_point timestamp;
    EventType type;
    Severity severity;
    std::string sourceIp;
    std::string username;
    std::string details;

    SecurityEvent(EventType t, Severity s, std::string ip, std::string user, std::string desc)
        : timestamp(std::chrono::system_clock::now()),
          type(t),
          severity(s),
          sourceIp(std::move(ip)),
          username(std::move(user)),
          details(std::move(desc)) {}
};

struct Alert {
    std::chrono::system_clock::time_point timestamp;
    std::string ruleName;
    Severity severity;
    std::string description;
};

std::string severityToString(Severity severity) {
    switch (severity) {
        case Severity::INFO: return "INFO";
        case Severity::WARNING: return "WARNING";
        case Severity::ERROR: return "ERROR";
        case Severity::CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

std::string eventTypeToString(EventType type) {
    switch (type) {
        case EventType::LOGIN_SUCCESS: return "LOGIN_SUCCESS";
        case EventType::LOGIN_FAILURE: return "LOGIN_FAILURE";
        case EventType::ACCESS_DENIED: return "ACCESS_DENIED";
        case EventType::PRIVILEGE_ESCALATION: return "PRIVILEGE_ESCALATION";
        case EventType::FILE_INTEGRITY_VIOLATION: return "FILE_INTEGRITY_VIOLATION";
        case EventType::SUSPICIOUS_NETWORK_ACTIVITY: return "SUSPICIOUS_NETWORK_ACTIVITY";
        default: return "UNKNOWN";
    }
}

std::string formatTimestamp(const std::chrono::system_clock::time_point& tp) {
    std::time_t time = std::chrono::system_clock::to_time_t(tp);
    std::tm tmBuffer{};
#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&tmBuffer, &time);
#else
    localtime_r(&time, &tmBuffer);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tmBuffer, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

class SecurityLogger {
private:
    std::mutex logMutex;
    std::vector<SecurityEvent> logHistory;

public:
    void logEvent(const SecurityEvent& event, std::ostream& outStream) {
        std::lock_guard<std::mutex> lock(logMutex);
        logHistory.push_back(event);
        outStream << "[" << formatTimestamp(event.timestamp) << "] "
                  << "[" << severityToString(event.severity) << "] "
                  << "[" << eventTypeToString(event.type) << "] "
                  << "IP: " << event.sourceIp << " | "
                  << "User: " << event.username << " | "
                  << "Details: " << event.details << "\n";
    }

    std::vector<SecurityEvent> getEvents() {
        std::lock_guard<std::mutex> lock(logMutex);
        return logHistory;
    }
};

class SecurityMonitor {
private:
    std::mutex monitorMutex;
    std::vector<Alert> activeAlerts;
    std::unordered_map<std::string, std::vector<std::chrono::system_clock::time_point>> failedLoginsByIp;
    
    const size_t BRUTE_FORCE_THRESHOLD = 3;
    const std::chrono::seconds BRUTE_FORCE_WINDOW = std::chrono::seconds(60);

    void pruneOldAttempts(std::vector<std::chrono::system_clock::time_point>& attempts, 
                           const std::chrono::system_clock::time_point& now) {
        attempts.erase(
            std::remove_if(attempts.begin(), attempts.end(),
                [&](const std::chrono::system_clock::time_point& t) {
                    return (now - t) > BRUTE_FORCE_WINDOW;
                }),
            attempts.end()
        );
    }

public:
    std::vector<Alert> evaluateEvent(const SecurityEvent& event) {
        std::lock_guard<std::mutex> lock(monitorMutex);
        std::vector<Alert> newAlerts;

        if (event.type == EventType::LOGIN_FAILURE) {
            auto& attempts = failedLoginsByIp[event.sourceIp];
            attempts.push_back(event.timestamp);
            pruneOldAttempts(attempts, event.timestamp);

            if (attempts.size() >= BRUTE_FORCE_THRESHOLD) {
                Alert alert{
                    event.timestamp,
                    "BRUTE_FORCE_SUSPECTED",
                    Severity::CRITICAL,
                    "Multiple failed logins (" + std::to_string(attempts.size()) + ") detected from IP: " + event.sourceIp
                };
                newAlerts.push_back(alert);
                activeAlerts.push_back(alert);
            }
        }

        if (event.type == EventType::PRIVILEGE_ESCALATION && event.severity == Severity::CRITICAL) {
            Alert alert{
                event.timestamp,
                "UNAUTHORIZED_PRIVILEGE_ESCALATION",
                Severity::CRITICAL,
                "Critical privilege escalation attempt detected by user: " + event.username
            };
            newAlerts.push_back(alert);
            activeAlerts.push_back(alert);
        }

        if (event.type == EventType::FILE_INTEGRITY_VIOLATION) {
            Alert alert{
                event.timestamp,
                "INTEGRITY_VIOLATION",
                Severity::ERROR,
                "File integrity violation reported: " + event.details
            };
            newAlerts.push_back(alert);
            activeAlerts.push_back(alert);
        }

        return newAlerts;
    }

    std::vector<Alert> getActiveAlerts() {
        std::lock_guard<std::mutex> lock(monitorMutex);
        return activeAlerts;
    }
};

class SecurityEngine {
private:
    SecurityLogger logger;
    SecurityMonitor monitor;

public:
    void processEvent(const SecurityEvent& event, std::ostream& outStream) {
        logger.logEvent(event, outStream);
        std::vector<Alert> alerts = monitor.evaluateEvent(event);
        for (const auto& alert : alerts) {
            outStream << ">>> [SECURITY ALERT] [" << severityToString(alert.severity) << "] "
                      << alert.ruleName << " - " << alert.description << " <<<\n";
        }
    }

    std::vector<Alert> getAllAlerts() {
        return monitor.getActiveAlerts();
    }
};

int main() {
    SecurityEngine engine;

    std::vector<SecurityEvent> eventsToSimulate = {
        SecurityEvent(EventType::LOGIN_SUCCESS, Severity::INFO, "192.168.1.10", "alice", "Successful login via SSH"),
        SecurityEvent(EventType::ACCESS_DENIED, Severity::WARNING, "192.168.1.50", "bob", "Unauthorized access to /etc/shadow"),
        SecurityEvent(EventType::LOGIN_FAILURE, Severity::WARNING, "10.0.0.99", "admin", "Incorrect password"),
        SecurityEvent(EventType::LOGIN_FAILURE, Severity::WARNING, "10.0.0.99", "admin", "Incorrect password"),
        SecurityEvent(EventType::LOGIN_FAILURE, Severity::WARNING, "10.0.0.99", "root", "Incorrect password"),
        SecurityEvent(EventType::FILE_INTEGRITY_VIOLATION, Severity::ERROR, "127.0.0.1", "system", "Checksum mismatch on /bin/bash"),
        SecurityEvent(EventType::PRIVILEGE_ESCALATION, Severity::CRITICAL, "192.168.1.15", "mallory", "Kernel exploit triggered")
    };

    std::cout << "--- Security Event Processing Started ---\n";
    for (const auto& event : eventsToSimulate) {
        engine.processEvent(event, std::cout);
    }
    std::cout << "--- Security Event Processing Finished ---\n\n";

    std::vector<Alert> allAlerts = engine.getAllAlerts();
    std::cout << "Total Active Alerts Triggered: " << allAlerts.size() << "\n";
    for (size_t i = 0; i < allAlerts.size(); ++i) {
        std::cout << (i + 1) << ". [" << formatTimestamp(allAlerts[i].timestamp) << "] "
                  << "[" << severityToString(allAlerts[i].severity) << "] "
                  << allAlerts[i].ruleName << ": " << allAlerts[i].description << "\n";
    }

    return 0;
}