import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
import java.time.Duration;
import java.util.*;
import java.util.concurrent.ConcurrentHashMap;
import java.util.stream.Collectors;
import java.io.*;

public class SecurityMonitorSystem {

    public enum Severity {
        INFO, WARNING, ERROR, CRITICAL
    }

    public enum EventType {
        LOGIN_ATTEMPT, LOGIN_SUCCESS, LOGIN_FAILURE, LOGOUT,
        FILE_ACCESS, PRIVILEGE_ESCALATION, NETWORK_CONNECTION,
        CONFIGURATION_CHANGE, MALWARE_DETECTED, BRUTE_FORCE_DETECTED,
        UNAUTHORIZED_ACCESS, DATA_EXFILTRATION, POLICY_VIOLATION
    }

    public static class SecurityEvent {
        private final String eventId;
        private final LocalDateTime timestamp;
        private final EventType type;
        private final Severity severity;
        private final String source;
        private final String user;
        private final String ipAddress;
        private final String description;
        private final Map<String, String> metadata;

        public SecurityEvent(EventType type, Severity severity, String source,
                           String user, String ipAddress, String description) {
            this.eventId = UUID.randomUUID().toString();
            this.timestamp = LocalDateTime.now();
            this.type = type;
            this.severity = severity;
            this.source = source;
            this.user = user;
            this.ipAddress = ipAddress;
            this.description = description;
            this.metadata = new HashMap<>();
        }

        public String getEventId() { return eventId; }
        public LocalDateTime getTimestamp() { return timestamp; }
        public EventType getType() { return type; }
        public Severity getSeverity() { return severity; }
        public String getSource() { return source; }
        public String getUser() { return user; }
        public String getIpAddress() { return ipAddress; }
        public String getDescription() { return description; }
        public Map<String, String> getMetadata() { return metadata; }

        public void addMetadata(String key, String value) {
            metadata.put(key, value);
        }

        @Override
        public String toString() {
            return String.format("[%s] %s | %s | %s | %s | %s | %s",
                timestamp.format(DateTimeFormatter.ISO_LOCAL_DATE_TIME),
                severity, type, source, user, ipAddress, description);
        }
    }

    public static class SecurityLogger {
        private final List<SecurityEvent> events;
        private final String logFilePath;
        private final Severity minimumSeverity;
        private final boolean enableConsoleOutput;

        public SecurityLogger(String logFilePath, Severity minimumSeverity, boolean enableConsoleOutput) {
            this.events = Collections.synchronizedList(new ArrayList<>());
            this.logFilePath = logFilePath;
            this.minimumSeverity = minimumSeverity;
            this.enableConsoleOutput = enableConsoleOutput;
        }

        public void logEvent(SecurityEvent event) {
            if (event.getSeverity().ordinal() >= minimumSeverity.ordinal()) {
                events.add(event);
                writeToFile(event);
                if (enableConsoleOutput) {