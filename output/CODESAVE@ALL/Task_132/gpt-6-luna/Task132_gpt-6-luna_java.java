import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.StandardOpenOption;
import java.time.Duration;
import java.time.Instant;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Deque;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.UUID;

public final class SecurityEventMonitor implements AutoCloseable {
    public enum EventType {
        LOGIN_SUCCESS,
        LOGIN_FAILURE,
        ACCESS_DENIED,
        PERMISSION_CHANGE,
        ACCOUNT_LOCKED,
        MALWARE_DETECTED,
        CONFIGURATION_CHANGE,
        CUSTOM
    }

    public enum Severity {
        INFO,
        WARNING,
        HIGH,
        CRITICAL
    }

    public static final class SecurityEvent {
        private final String id;
        private final Instant timestamp;
        private final EventType type;
        private final Severity severity;
        private final String actor;
        private final String source;
        private final String resource;
        private final String details;

        private SecurityEvent(
                String id,
                Instant timestamp,
                EventType type,
                Severity severity,
                String actor,
                String source,
                String resource,
                String details) {
            this.id = id;
            this.timestamp = timestamp;
            this.type = type;
            this.severity = severity;
            this.actor = actor;
            this.source = source;
            this.resource = resource;
            this.details = details;
        }
    }

    public static final class Alert {
        private final Instant timestamp;
        private final Severity severity;
        private final String message;
        private final String eventId;

        private Alert(Instant timestamp, Severity severity, String message, String eventId) {
            this.timestamp = timestamp;
            this.severity = severity;
            this.message = message;
            this.eventId = eventId;
        }
    }

    private static final class FailureWindow {
        private final Deque<Instant> timestamps = new ArrayDeque<Instant>();
    }

    private final BufferedWriter logWriter;
    private final int failureThreshold;
    private final Duration failureWindow;
    private final Map<String, FailureWindow> failuresByIdentity =
            new HashMap<String, FailureWindow>();

    public SecurityEventMonitor(Path logPath, int failureThreshold, Duration failureWindow)
            throws IOException {
        if (logPath == null) {
            throw new IllegalArgumentException("Log path must not be null");
        }
        if (failureThreshold < 1) {
            throw new IllegalArgumentException("Failure threshold must be at least 1");
        }
        if (failureWindow == null || failureWindow.isZero() || failureWindow.isNegative()) {
            throw new IllegalArgumentException("Failure window must be positive");
        }

        Path absolutePath = logPath.toAbsolutePath();
        Path parent = absolutePath.getParent();
        if (parent != null) {
            Files.createDirectories(parent);
        }

        this.logWriter = Files.newBufferedWriter(
                absolutePath,
                StandardCharsets.UTF_8,
                StandardOpenOption.CREATE,
                StandardOpenOption.APPEND);
        this.failureThreshold = failureThreshold;
        this.failureWindow = failureWindow;
    }

    public synchronized List<Alert> recordEvent(
            String eventType,
            String actor,
            String source,
            String resource,
            String details) throws IOException {
        if (eventType == null || eventType.trim().isEmpty()) {
            throw new IllegalArgumentException("Event type must not be empty");
        }

        EventType parsedType;
        try {
            parsedType = EventType.valueOf(eventType.trim().toUpperCase(Locale.ROOT));
        } catch (IllegalArgumentException exception) {
            throw new IllegalArgumentException("Unsupported event type: " + eventType, exception);
        }

        Instant now = Instant.now();
        SecurityEvent event = new SecurityEvent(
                UUID.randomUUID().toString(),
                now,
                parsedType,
                severityFor(parsedType),
                clean(actor),
                clean(source),
                clean(resource),
                clean(details));

        writeLine(toJson(event));
        List<Alert> alerts = evaluate(event);
        for (Alert alert : alerts) {
            writeLine(toJson(alert));
            System.out.println(
                    alert.timestamp + " " + alert.severity + " ALERT: " + alert.message);
        }
        logWriter.flush();
        return Collections.unmodifiableList(alerts);
    }

    private List<Alert> evaluate(SecurityEvent event) {
        List<Alert> alerts = new ArrayList<Alert>();

        if (event.type == EventType.LOGIN_FAILURE) {
            String identity = event.actor + "\u0000" + event.source;
            FailureWindow failureWindowForIdentity = failuresByIdentity.get(identity);
            if (failureWindowForIdentity == null) {
                failureWindowForIdentity = new FailureWindow();
                failuresByIdentity.put(identity, failureWindowForIdentity);
            }

            Instant cutoff = event.timestamp.minus(failureWindow);
            while (!failureWindowForIdentity.timestamps.isEmpty()
                    && failureWindowForIdentity.timestamps.peekFirst().isBefore(cutoff)) {
                failureWindowForIdentity.timestamps.removeFirst();
            }
            failureWindowForIdentity.timestamps.addLast(event.timestamp);

            if (failureWindowForIdentity.timestamps.size() == failureThreshold) {
                alerts.add(new Alert(
                        event.timestamp,
                        Severity.HIGH,
                        "Repeated login failures for actor '" + event.actor
                                + "' from source '" + event.source + "' ("
                                + failureThreshold + " within " + failureWindow.getSeconds()
                                + " seconds)",
                        event.id));
            }
        } else if (event.type == EventType.ACCESS_DENIED) {
            alerts.add(new Alert(
                    event.timestamp,
                    Severity.WARNING,
                    "Access denied for actor '" + event.actor + "' to resource '"
                            + event.resource + "'",
                    event.id));
        } else if (event.type == EventType.ACCOUNT_LOCKED) {
            alerts.add(new Alert(
                    event.timestamp,
                    Severity.HIGH,
                    "Account locked for actor '" + event.actor + "'",
                    event.id));
        } else if (event.type == EventType.MALWARE_DETECTED) {
            alerts.add(new Alert(
                    event.timestamp,
                    Severity.CRITICAL,
                    "Malware detected for actor '" + event.actor + "' on resource '"
                            + event.resource + "'",
                    event.id));
        } else if (event.type == EventType.PERMISSION_CHANGE) {
            alerts.add(new Alert(
                    event.timestamp,
                    Severity.WARNING,
                    "Permission change recorded for actor '" + event.actor
                            + "' on resource '" + event.resource + "'",
                    event.id));
        }

        return alerts;
    }

    private static Severity severityFor(EventType type) {
        switch (type) {
            case LOGIN_FAILURE:
            case ACCESS_DENIED:
                return Severity.WARNING;
            case ACCOUNT_LOCKED:
            case PERMISSION_CHANGE:
            case CONFIGURATION_CHANGE:
                return Severity.HIGH;
            case MALWARE_DETECTED:
                return Severity.CRITICAL;
            case LOGIN_SUCCESS:
            case CUSTOM:
            default:
                return Severity.INFO;
        }
    }

    private void writeLine(String line) throws IOException {
        logWriter.write(line);
        logWriter.newLine();
    }

    private static String toJson(SecurityEvent event) {
        return "{"
                + "\"recordType\":\"event\","
                + "\"id\":\"" + escapeJson(event.id) + "\","
                + "\"timestamp\":\"" + escapeJson(event.timestamp.toString()) + "\","
                + "\"type\":\"" + escapeJson(event.type.name()) + "\","
                + "\"severity\":\"" + escapeJson(event.severity.name()) + "\","
                + "\"actor\":\"" + escapeJson(event.actor) + "\","
                + "\"source\":\"" + escapeJson(event.source) + "\","
                + "\"resource\":\"" + escapeJson(event.resource) + "\","
                + "\"details\":\"" + escapeJson(event.details) + "\""
                + "}";
    }

    private static String toJson(Alert alert) {
        return "{"
                + "\"recordType\":\"alert\","
                + "\"timestamp\":\"" + escapeJson(alert.timestamp.toString()) + "\","
                + "\"severity\":\"" + escapeJson(alert.severity.name()) + "\","
                + "\"message\":\"" + escapeJson(alert.message) + "\","
                + "\"eventId\":\"" + escapeJson(alert.eventId) + "\""
                + "}";
    }

    private static String escapeJson(String value) {
        StringBuilder escaped = new StringBuilder();
        for (int i = 0; i < value.length(); i++) {
            char character = value.charAt(i);
            switch (character) {
                case '"':
                    escaped.append("\\\"");
                    break;
                case '\\':
                    escaped.append("\\\\");
                    break;
                case '\b':
                    escaped.append("\\b");
                    break;
                case '\f':
                    escaped.append("\\f");
                    break;
                case '\n':
                    escaped.append("\\n");
                    break;
                case '\r':
                    escaped.append("\\r");
                    break;
                case '\t':
                    escaped.append("\\t");
                    break;
                default:
                    if (character < 0x20) {
                        escaped.append(String.format("\\u%04x", (int) character));
                    } else {
                        escaped.append(character);
                    }
            }
        }
        return escaped.toString();
    }

    private static String clean(String value) {
        return value == null ? "" : value.trim();
    }

    @Override
    public synchronized void close() throws IOException {
        logWriter.flush();
        logWriter.close();
    }

    public static void main(String[] args) throws Exception {
        Path logPath = Paths.get("security-events.jsonl");
        int failureThreshold = 5;
        long windowSeconds = 300;

        for (int i = 0; i < args.length; i++) {
            if ("--log".equals(args[i]) && i + 1 < args.length) {
                logPath = Paths.get(args[++i]);
            } else if ("--threshold".equals(args[i]) && i + 1 < args.length) {
                failureThreshold = Integer.parseInt(args[++i]);
            } else if ("--window-seconds".equals(args[i]) && i + 1 < args.length) {
                windowSeconds = Long.parseLong(args[++i]);
            } else {
                System.err.println(
                        "Usage: java SecurityEventMonitor [--log path] "
                                + "[--threshold count] [--window-seconds seconds]");
                System.err.println(
                        "Enter events on stdin as TYPE|ACTOR|SOURCE|RESOURCE|DETAILS");
                return;
            }
        }

        if (windowSeconds < 1) {
            throw new IllegalArgumentException("Window duration must be at least one second");
        }

        try (SecurityEventMonitor monitor = new SecurityEventMonitor(
                logPath, failureThreshold, Duration.ofSeconds(windowSeconds));
             BufferedReader input = new BufferedReader(
                     new InputStreamReader(System.in, StandardCharsets.UTF_8))) {
            String line;
            while ((line = input.readLine()) != null) {
                if (line.trim().isEmpty()) {
                    continue;
                }
                String[] fields = line.split("\\|", 5);
                if (fields.length < 1 || fields[0].trim().isEmpty()) {
                    System.err.println("Ignoring malformed event line");
                    continue;
                }

                String actor = fields.length > 1 ? fields[1] : "";
                String source = fields.length > 2 ? fields[2] : "";
                String resource = fields.length > 3 ? fields[3] : "";
                String details = fields.length > 4 ? fields[4] : "";

                try {
                    monitor.recordEvent(fields[0], actor, source, resource, details);
                } catch (IllegalArgumentException exception) {
                    System.err.println("Ignoring event: " + exception.getMessage());
                } catch (IOException exception) {
                    System.err.println("Failed to write event log: " + exception.getMessage());
                    throw exception;
                }
            }
        }
    }
}