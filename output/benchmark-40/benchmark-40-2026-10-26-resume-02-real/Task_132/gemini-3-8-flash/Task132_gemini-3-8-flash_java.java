import java.time.Instant;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Deque;
import java.util.EnumMap;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.stream.Collectors;

public class SecurityMonitoringSystem {

    public enum Severity {
        INFO, LOW, MEDIUM, HIGH, CRITICAL
    }

    public enum EventType {
        LOGIN_SUCCESS,
        LOGIN_FAILURE,
        ACCESS_DENIED,
        PRIVILEGE_ESCALATION_ATTEMPT,
        SENSITIVE_DATA_ACCESS,
        SUSPICIOUS_PAYLOAD,
        ACCOUNT_LOCKED
    }

    public static final class SecurityEvent {
        private final String eventId;
        private final Instant timestamp;
        private final EventType type;
        private final Severity severity;
        private final String userId;
        private final String ipAddress;
        private final String resource;
        private final String details;

        public SecurityEvent(String eventId, Instant timestamp, EventType type, Severity severity,
                             String userId, String ipAddress, String resource, String details) {
            this.eventId = Objects.requireNonNull(eventId, "eventId cannot be null");
            this.timestamp = Objects.requireNonNull(timestamp, "timestamp cannot be null");
            this.type = Objects.requireNonNull(type, "type cannot be null");
            this.severity = Objects.requireNonNull(severity, "severity cannot be null");
            this.userId = userId != null ? userId : "ANONYMOUS";
            this.ipAddress = ipAddress != null ? ipAddress : "0.0.0.0";
            this.resource = resource != null ? resource : "N/A";
            this.details = details != null ? details : "";
        }

        public String getEventId() { return eventId; }
        public Instant getTimestamp() { return timestamp; }
        public EventType getType() { return type; }
        public Severity getSeverity() { return severity; }
        public String getUserId() { return userId; }
        public String getIpAddress() { return ipAddress; }
        public String getResource() { return resource; }
        public String getDetails() { return details; }

        @Override
        public String toString() {
            return String.format("[%s] %s | Severity: %-8s | User: %-10s | IP: %-15s | Resource: %-15s | %s",
                    timestamp, type, severity, userId, ipAddress, resource, details);
        }
    }

    public static final class SecurityAlert {
        private final String alertId;
        private final Instant timestamp;
        private final String alertName;
        private final Severity severity;
        private final String description;
        private final List<SecurityEvent> triggeringEvents;

        public SecurityAlert(String alertId, Instant timestamp, String alertName,
                             Severity severity, String description, List<SecurityEvent> triggeringEvents) {
            this.alertId = alertId;
            this.timestamp = timestamp;
            this.alertName = alertName;
            this.severity = severity;
            this.description = description;
            this.triggeringEvents = Collections.unmodifiableList(new ArrayList<>(triggeringEvents));
        }

        public String getAlertId() { return alertId; }
        public Instant getTimestamp() { return timestamp; }
        public String getAlertName() { return alertName; }
        public Severity getSeverity() { return severity; }
        public String getDescription() { return description; }
        public List<SecurityEvent> getTriggeringEvents() { return triggeringEvents; }

        @Override
        public String toString() {
            return String.format("ALERT [%s] %s | Severity: %s | Description: %s (Triggered by %d events)",
                    timestamp, alertName, severity, description, triggeringEvents.size());
        }
    }

    public interface SecurityRule {
        List<SecurityAlert> evaluate(SecurityEvent incomingEvent, List<SecurityEvent> historicalEvents);
    }

    public static class BruteForceLoginRule implements SecurityRule {
        private final int thresholdAttempts;
        private final long windowSeconds;

        public BruteForceLoginRule(int thresholdAttempts, long windowSeconds) {
            this.thresholdAttempts = thresholdAttempts;
            this.windowSeconds = windowSeconds;
        }

        @Override
        public List<SecurityAlert> evaluate(SecurityEvent incomingEvent, List<SecurityEvent> historicalEvents) {
            if (incomingEvent.getType() != EventType.LOGIN_FAILURE) {
                return Collections.emptyList();
            }

            Instant cutoff = incomingEvent.getTimestamp().minusSeconds(windowSeconds);
            List<SecurityEvent> relevantFailures = historicalEvents.stream()
                    .filter(e -> e.getType() == EventType.LOGIN_FAILURE)
                    .filter(e -> !e.getTimestamp().isBefore(cutoff))
                    .filter(e -> e.getIpAddress().equals(incomingEvent.getIpAddress()))
                    .collect(Collectors.toList());

            if (relevantFailures.size() >= thresholdAttempts) {
                SecurityAlert alert = new SecurityAlert(
                        "ALT-BF-" + System.nanoTime(),
                        incomingEvent.getTimestamp(),
                        "BRUTE_FORCE_DETECTED",
                        Severity.HIGH,
                        String.format("Potential brute force attack detected from IP %s: %d failed logins in %d seconds.",
                                incomingEvent.getIpAddress(), relevantFailures.size(), windowSeconds),
                        relevantFailures
                );
                return Collections.singletonList(alert);
            }
            return Collections.emptyList();
        }
    }

    public static class PrivilegeEscalationRule implements SecurityRule {
        @Override
        public List<SecurityAlert> evaluate(SecurityEvent incomingEvent, List<SecurityEvent> historicalEvents) {
            if (incomingEvent.getType() == EventType.PRIVILEGE_ESCALATION_ATTEMPT) {
                SecurityAlert alert = new SecurityAlert(
                        "ALT-PE-" + System.nanoTime(),
                        incomingEvent.getTimestamp(),
                        "PRIVILEGE_ESCALATION_DETECTED",
                        Severity.CRITICAL,
                        String.format("Unauthorized privilege escalation attempt by user '%s' on resource '%s'.",
                                incomingEvent.getUserId(), incomingEvent.getResource()),
                        Collections.singletonList(incomingEvent)
                );
                return Collections.singletonList(alert);
            }
            return Collections.emptyList();
        }
    }

    public static class RepeatedAccessDeniedRule implements SecurityRule {
        private final int threshold;
        private final long windowSeconds;

        public RepeatedAccessDeniedRule(int threshold, long windowSeconds) {
            this.threshold = threshold;
            this.windowSeconds = windowSeconds;
        }

        @Override
        public List<SecurityAlert> evaluate(SecurityEvent incomingEvent, List<SecurityEvent> historicalEvents) {
            if (incomingEvent.getType() != EventType.ACCESS_DENIED) {
                return Collections.emptyList();
            }

            Instant cutoff = incomingEvent.getTimestamp().minusSeconds(windowSeconds);
            List<SecurityEvent> deniedEvents = historicalEvents.stream()
                    .filter(e -> e.getType() == EventType.ACCESS_DENIED)
                    .filter(e -> !e.getTimestamp().isBefore(cutoff))
                    .filter(e -> e.getUserId().equals(incomingEvent.getUserId()))
                    .collect(Collectors.toList());

            if (deniedEvents.size() >= threshold) {
                SecurityAlert alert = new SecurityAlert(
                        "ALT-AD-" + System.nanoTime(),
                        incomingEvent.getTimestamp(),
                        "REPEATED_ACCESS_DENIED",
                        Severity.MEDIUM,
                        String.format("User '%s' experienced %d unauthorized access denials in %d seconds.",
                                incomingEvent.getUserId(), deniedEvents.size(), windowSeconds),
                        deniedEvents
                );
                return Collections.singletonList(alert);
            }
            return Collections.emptyList();
        }
    }

    private final List<SecurityEvent> auditLog = new CopyOnWriteArrayList<>();
    private final List<SecurityAlert> alertLog = new CopyOnWriteArrayList<>();
    private final List<SecurityRule> activeRules = new ArrayList<>();

    public SecurityMonitoringSystem() {
        registerRule(new BruteForceLoginRule(3, 60));
        registerRule(new PrivilegeEscalationRule());
        registerRule(new RepeatedAccessDeniedRule(3, 30));
    }

    public void registerRule(SecurityRule rule) {
        if (rule != null) {
            this.activeRules.add(rule);
        }
    }

    public synchronized List<SecurityAlert> logAndProcessEvent(EventType type, Severity severity,
                                                               String userId, String ipAddress,
                                                               String resource, String details) {
        String eventId = "EVT-" + System.currentTimeMillis() + "-" + (auditLog.size() + 1);
        SecurityEvent event = new SecurityEvent(eventId, Instant.now(), type, severity, userId, ipAddress, resource, details);
        auditLog.add(event);

        List<SecurityAlert> generatedAlerts = new ArrayList<>();
        for (SecurityRule rule : activeRules) {
            List<SecurityAlert> alerts = rule.evaluate(event, auditLog);
            if (alerts != null && !alerts.isEmpty()) {
                generatedAlerts.addAll(alerts);
            }
        }

        alertLog.addAll(generatedAlerts);
        return Collections.unmodifiableList(generatedAlerts);
    }

    public List<SecurityEvent> getAuditLog() {
        return Collections.unmodifiableList(auditLog);
    }

    public List<SecurityAlert> getAlertLog() {
        return Collections.unmodifiableList(alertLog);
    }

    public Map<Severity, Long> getAlertCountBySeverity() {
        Map<Severity, Long> counts = new EnumMap<>(Severity.class);
        for (Severity s : Severity.values()) {
            counts.put(s, 0L);
        }
        for (SecurityAlert alert : alertLog) {
            counts.put(alert.getSeverity(), counts.get(alert.getSeverity()) + 1);
        }
        return counts;
    }

    public List<SecurityEvent> queryEventsByUser(String userId) {
        return auditLog.stream()
                .filter(e -> e.getUserId().equalsIgnoreCase(userId))
                .collect(Collectors.toList());
    }

    public List<SecurityEvent> queryEventsByIp(String ipAddress) {
        return auditLog.stream()
                .filter(e -> e.getIpAddress().equalsIgnoreCase(ipAddress))
                .collect(Collectors.toList());
    }

    public static void main(String[] args) {
        SecurityMonitoringSystem monitor = new SecurityMonitoringSystem();

        System.out.println("=== 1. Simulating Normal Activity ===");
        monitor.logAndProcessEvent(
                EventType.LOGIN_SUCCESS, Severity.INFO,
                "alice", "192.168.1.10", "/auth/login", "Standard user login"
        );
        monitor.logAndProcessEvent(
                EventType.SENSITIVE_DATA_ACCESS, Severity.LOW,
                "alice", "192.168.1.10", "/api/v1/profile", "Read own profile data"
        );

        System.out.println("\n=== 2. Simulating Brute Force Attack ===");
        String attackerIp = "203.0.113.55";
        for (int i = 1; i <= 3; i++) {
            List<SecurityAlert> alerts = monitor.logAndProcessEvent(
                    EventType.LOGIN_FAILURE, Severity.LOW,
                    "admin", attackerIp, "/auth/login", "Invalid credentials, attempt " + i
            );
            System.out.printf("Attempt %d logged. Triggered alerts: %d%n", i, alerts.size());
            for (SecurityAlert alert : alerts) {
                System.out.println("  >>> " + alert);
            }
        }

        System.out.println("\n=== 3. Simulating Privilege Escalation ===");
        List<SecurityAlert> peAlerts = monitor.logAndProcessEvent(
                EventType.PRIVILEGE_ESCALATION_ATTEMPT, Severity.CRITICAL,
                "bob", "192.168.1.25", "/admin/sudo", "Attempted root role assignment via parameter tampering"
        );
        for (SecurityAlert alert : peAlerts) {
            System.out.println("  >>> " + alert);
        }

        System.out.println("\n=== 4. Simulating Repeated Access Denials ===");
        for (int i = 1; i <= 3; i++) {
            List<SecurityAlert> alerts = monitor.logAndProcessEvent(
                    EventType.ACCESS_DENIED, Severity.MEDIUM,
                    "charlie", "10.0.0.12", "/confidential/payroll", "HTTP 403 Forbidden - Insufficient scope"
            );
            for (SecurityAlert alert : alerts) {
                System.out.println("  >>> " + alert);
            }
        }

        System.out.println("\n=== Summary of Audit Log (" + monitor.getAuditLog().size() + " events) ===");
        for (SecurityEvent event : monitor.getAuditLog()) {
            System.out.println(event);
        }

        System.out.println("\n=== Summary of Alerts (" + monitor.getAlertLog().size() + " alerts) ===");
        for (SecurityAlert alert : monitor.getAlertLog()) {
            System.out.println(alert);
        }

        System.out.println("\n=== Alert Distribution by Severity ===");
        Map<Severity, Long> distribution = monitor.getAlertCountBySeverity();
        distribution.forEach((sev, count) -> System.out.printf("%-10s : %d%n", sev, count));
    }
}