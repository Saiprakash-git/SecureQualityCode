#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>
#include <map>
#include <algorithm>
#include <iomanip>

enum class EventType {
    AUTHENTICATION,
    AUTHORIZATION,
    FILE_ACCESS,
    NETWORK_CONNECTION,
    PRIVILEGE_ESCALATION,
    SYSTEM_CHANGE,
    MALWARE_DETECTION,
    CONFIGURATION_CHANGE,
    DATA_EXFILTRATION,
    INTRUSION_ATTEMPT
};

enum class SeverityLevel {
    INFO,
    LOW,
    MEDIUM,
    HIGH,
    CRITICAL
};

struct SecurityEvent {
    std::string eventId;
    std::chrono::system_clock::time_point timestamp;
    EventType type;
    SeverityLevel severity;
    std::string source;
    std::string description;
    std::string user;
    std::string ipAddress;
    std::map<std::string, std::string> metadata;

    SecurityEvent(EventType t, SeverityLevel s, const std::string& src,
                  const std::string& desc, const std::string& usr = "",
                  const std::string& ip = "")
        : type(t), severity(s), source(src), description(desc),
          user(usr), ipAddress(ip) {
        timestamp = std::chrono::system_clock::now();
        static std::atomic<int> counter{0};
        eventId = "EVT-" + std::to_string(++counter);
    }
};

class SecurityLogger {
private:
    std::ofstream logFile;
    mutable std::mutex logMutex;
    std::string logFilePath;
    bool consoleOutput;

    std::string getCurrentTimestamp() const {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

    std::string severityToString(SeverityLevel level) const {
        switch(level) {
            case SeverityLevel::INFO: