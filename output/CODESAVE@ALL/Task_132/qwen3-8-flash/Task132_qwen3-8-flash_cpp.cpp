#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <cstdint>

enum class SecuritySeverity : int {
    Info = 0,
    Warning = 1,
    Critical = 2
};

struct SecurityEvent {
    std::string id;
    std::string timestamp;
    std::string source;
    std::string message;
    SecuritySeverity severity;
};

std::string currentTimestamp() {
    std::time_t now = std::time(nullptr);
    std::tm* local = std::localtime(&now);
    char buffer[64];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", local);
    return std::string(buffer);
}

std::string severityToString(SecuritySeverity severity) {
    switch (severity) {
        case SecuritySeverity::Info:
            return "INFO";
        case SecuritySeverity::Warning:
            return "WARNING";
        case SecuritySeverity::Critical:
            return "CRITICAL";
        default:
            return "UNKNOWN";
    }
}

std::string createEventId(std::uint64_t counter) {
    std::ostringstream out;
    out << "EVT-" << std::setw(5) << std::setfill('0') << counter;
    return out.str();
}

SecurityEvent createSecurityEvent(
    const std::string& source,
    const std::string& message,
    SecuritySeverity severity,
    std::uint64_t counter
) {
    SecurityEvent event;
    event.id = createEventId(counter);
    event.timestamp = currentTimestamp();
    event.source = source;
    event.message = message;
    event.severity = severity;
    return event;
}

std::string formatSecurityEvent(const SecurityEvent& event) {
    std::ostringstream out;
    out << "[" << event.timestamp << "] "
        << "[" << event.id << "] "
        << "[" << severityToString(event.severity) << "] "
        << event.source << ": "
        << event.message;
    return out.str();
}

bool isCriticalSecurityEvent(const SecurityEvent& event) {
    return event.severity == SecuritySeverity::Critical;
}

bool isWarningSecurityEvent(const SecurityEvent& event) {
    return event.severity == SecuritySeverity::Warning;
}

std::vector<SecurityEvent> filterSecurityEventsBySeverity(
    const std::vector<SecurityEvent>& events,
    SecuritySeverity severity
) {
    std::vector<SecurityEvent> filtered;
    for (const SecurityEvent& event : events) {
        if (event.severity == severity) {
            filtered.push_back(event);
        }
    }
    return filtered;
}

std::string monitorSecurityEvent(
    const SecurityEvent& event,
    const std::string& previousAlertState
) {
    if (isCriticalSecurityEvent(event)) {
        return "ALERT: " + formatSecurityEvent(event);
    }

    if (isWarningSecurityEvent(event) && previousAlertState != "alerted") {
        return "WATCH: " + formatSecurityEvent(event);
    }

    return "";
}

std::string summarizeSecurityEvents(const std::vector<SecurityEvent>& events) {
    std::size_t infoCount = 0;
    std::size_t warningCount = 0;
    std::size_t criticalCount = 0;

    for (const SecurityEvent& event : events) {
        if (event.severity == SecuritySeverity::Info) {
            ++infoCount;
        } else if (event.severity == SecuritySeverity::Warning) {
            ++warningCount;
        } else if (event.severity == SecuritySeverity::Critical) {
            ++criticalCount;
        }
    }

    std::ostringstream out;
    out << "Total events: " << events.size()
        << " | Info: " << infoCount
        << " | Warning: " << warningCount
        << " | Critical: " << criticalCount;
    return out.str();
}

int main() {
    std::vector<SecurityEvent> events;
    std::uint64_t counter = 1;

    events.push_back(createSecurityEvent(
        "auth-service",
        "User login succeeded",
        SecuritySeverity::Info,
        counter++
    ));

    events.push_back(createSecurityEvent(
        "firewall",
        "Port scan detected from 203.0.113.10",
        SecuritySeverity::Warning,
        counter++
    ));

    events.push_back(createSecurityEvent(
        "admin-console",
        "Privilege escalation attempt blocked",
        SecuritySeverity::Critical,
        counter++
    ));

    events.push_back(createSecurityEvent(
        "file-integrity",
        "Configuration file modified",
        SecuritySeverity::Warning,
        counter++
    ));

    events.push_back(createSecurityEvent(
        "ids",
        "Malware signature match",
        SecuritySeverity::Critical,
        counter++
    ));

    std::cout << "Security Event Log\n";
    std::cout << "------------------\n";
    for (const SecurityEvent& event : events) {
        std::cout << formatSecurityEvent(event) << "\n";
    }

    std::cout << "\nMonitoring Results\n";
    std::cout << "------------------\n";
    std::string alertState = "none";
    for (const SecurityEvent& event : events) {
        std::string action = monitorSecurityEvent(event, alertState);
        if (!action.empty()) {
            std::cout << action << "\n";
            alertState = "alerted";
        }
    }

    std::cout << "\nSummary\n";
    std::cout << "-------\n";
    std::cout << summarizeSecurityEvents(events) << "\n";

    return 0;
}