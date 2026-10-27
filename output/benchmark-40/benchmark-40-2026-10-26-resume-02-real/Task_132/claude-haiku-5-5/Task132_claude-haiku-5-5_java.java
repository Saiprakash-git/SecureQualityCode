import java.io.*;
import java.nio.file.*;
import java.time.*;
import java.time.format.DateTimeFormatter;
import java.util.*;
import java.util.concurrent.*;
import java.util.concurrent.atomic.*;
import java.util.stream.*;

public class SecurityEventMonitor {

    enum Severity { INFO, LOW, MEDIUM, HIGH, CRITICAL }

    static final class SecurityEvent {
        final Instant timestamp;
        final String source;
        final String type;
        final Severity severity;
        final String user;
        final String ipAddress;
        final String message;

        SecurityEvent(Instant timestamp, String source, String type, Severity severity,
                      String user, String ipAddress, String message) {
            this.timestamp = timestamp;
            this.source = source;
            this.type = type;
            this.severity = severity;
            this.user = user;
            this.ipAddress = ipAddress;
            this.message = message;
        }

        String toLogLine() {
            return String.join("|",
                    DateTimeFormatter.ISO_INSTANT.format(timestamp),
                    severity.name(),
                    sanitize(source),
                    sanitize(type),
                    sanitize(user),
                    sanitize(ipAddress),
                    sanitize(message));
        }

        static SecurityEvent fromLogLine(String line) {
            String[] p = line.split("\\|", 7);
            if (p.length < 7) throw new IllegalArgumentException("Malformed log line: " + line);
            return new SecurityEvent(Instant.parse(p[0]), p[2], p[3], Severity.valueOf(p[1]),
                    p[4], p[5], p[6]);
        }

        private static String sanitize(String s) {
            if (s == null) return "";
            return s.replace("\\", "\\\\").replace("|", "\\p").replace("\n", "\\n").replace("\r", "\\r");
        }

        @Override
        public String toString() {
            return toLogLine();
        }
    }

    interface Rule {
        String name();
        Optional<SecurityEvent> evaluate(SecurityEvent event, List<SecurityEvent> history);
    }

    static final class ThresholdRule implements Rule {
        private final String name;
        private final String eventType;
        private final int threshold;
        private final Duration window;
        private final Severity alertSeverity;

        ThresholdRule(String name, String eventType, int threshold, Duration window, Severity alertSeverity) {
            this.name = name;
            this.eventType = eventType;
            this.threshold = threshold;
            this.window = window;
            this.alertSeverity = alertSeverity;
        }

        public String name() { return name; }

        public Optional<SecurityEvent> evaluate(SecurityEvent event, List<SecurityEvent> history) {
            if (!eventType.equals(event.type)) return Optional.empty();
            Instant cutoff = event.timestamp.minus(window);
            long count = history.stream()
                    .filter(e -> eventType.equals(e.type))
                    .filter(e -> e.ipAddress.equals(event.ipAddress))
                    .filter(e -> !e.timestamp.isBefore(cutoff) && !e.timestamp.isAfter(event.timestamp))
                    .count();
            if (count >= threshold) {
                return Optional.of(new SecurityEvent(event.timestamp, "MONITOR", "ALERT", alertSeverity,
                        event.user, event.ipAddress,
                        name + ": " + count + " '" + eventType + "' events from " + event.ipAddress
                                + " within " + window.toSeconds() + "s"));
            }
            return Optional.empty();
        }
    }

    static final class SeverityRule implements Rule {
        public String name() { return "CRITICAL_EVENT"; }

        public Optional<SecurityEvent> evaluate(SecurityEvent event, List<SecurityEvent> history) {
            if (event.severity == Severity.CRITICAL || event.severity == Severity.HIGH) {
                return Optional.of(new SecurityEvent(event.timestamp, "MONITOR", "ALERT", event.severity,
                        event.user, event.ipAddress,
                        "High-severity event detected: " + event.type + " - " + event.message));
            }
            return Optional.empty();
        }
    }

    static final class SecurityEventLogger implements AutoCloseable {
        private final Path logFile;
        private final BufferedWriter writer;
        private final List<SecurityEvent> history = new ArrayList<>();
        private final List<Rule> rules = new ArrayList<>();
        private final List<SecurityEvent> alerts = new ArrayList<>();
        private final int maxHistory;
        private final Object lock = new Object();
        private final AtomicLong counter = new AtomicLong();

        SecurityEventLogger(Path logFile, int maxHistory) throws IOException {
            this.logFile = logFile;
            this.maxHistory = maxHistory;
            Path parent = logFile.toAbsolutePath().getParent();
            if (parent != null) Files.createDirectories(parent);
            this.writer = Files.newBufferedWriter(logFile, StandardCharsets.UTF_8,
                    StandardOpenOption.CREATE, StandardOpenOption.APPEND);
        }

        void addRule(Rule rule) {
            synchronized (lock) { rules.add(rule); }
        }

        List<SecurityEvent> log(String source, String type, Severity severity,
                                String user, String ip, String message) throws IOException {
            return log(new SecurityEvent(Instant.now(), source, type, severity, user, ip, message));
        }

        List<SecurityEvent> log(SecurityEvent event) throws IOException {
            List<SecurityEvent> generated = new ArrayList<>();
            synchronized (lock) {
                writer.write(event.toLogLine());
                writer.newLine();
                writer.flush();
                counter.incrementAndGet();

                for (Rule rule : rules) {
                    rule.evaluate(event, history).ifPresent(alert -> {
                        generated.add(alert);
                    });
                }
                for (SecurityEvent alert : generated) {
                    alerts.add(alert);
                    writer.write(alert.toLogLine());
                    writer.newLine();
                }
                writer.flush();

                history.add(event);
                if (history.size() > maxHistory) {
                    history.subList(0, history.size() - maxHistory).clear();
                }
            }
            return generated;
        }

        List<SecurityEvent> getAlerts() {
            synchronized (lock) { return new ArrayList<>(alerts); }
        }

        long getEventCount() { return counter.get(); }

        @Override
        public void close() throws IOException {
            synchronized (lock) { writer.close(); }
        }
    }

    static List<SecurityEvent> readLog(Path logFile) throws IOException {
        if (!Files.exists(logFile)) return Collections.emptyList();
        List<SecurityEvent> events = new ArrayList<>();
        for (String line : Files.readAllLines(logFile, StandardCharsets.UTF_8)) {
            if (line.isBlank()) continue;
            try {
                events.add(SecurityEvent.fromLogLine(line));
            } catch (RuntimeException ex) {
                // skip malformed lines
            }
        }
        return events;
    }

    static Map<String, Long> summarizeByType(List<SecurityEvent> events) {
        return events.stream().collect(Collectors.groupingBy(e -> e.type, TreeMap::new, Collectors.counting()));
    }

    static Map<Severity, Long> summarizeBySeverity(List<SecurityEvent> events) {
        return events.stream().collect(Collectors.groupingBy(e -> e.severity, () -> new EnumMap<>(Severity.class), Collectors.counting()));
    }

    static List<SecurityEvent> filterByMinSeverity(List<SecurityEvent> events, Severity min) {
        return events.stream().filter(e -> e.severity.compareTo(min) >= 0).collect(Collectors.toList());
    }

    static String buildReport(List<SecurityEvent> events) {
        StringBuilder sb = new StringBuilder();
        sb.append("Security Event Report\n");
        sb.append("Total events: ").append(events.size()).append("\n");
        sb.append("\nBy severity:\n");
        summarizeBySeverity(events).forEach((k, v) -> sb.append("  ").append(k).append(": ").append(v).append("\n"));
        sb.append("\nBy type:\n");
        summarizeByType(events).forEach((k, v) -> sb.append("  ").append(k).append(": ").append(v).append("\n"));
        Map<String, Long> topIps = events.stream()
                .filter(e -> e.ipAddress != null && !e.ipAddress.isEmpty())
                .collect(Collectors.groupingBy(e -> e.ipAddress, Collectors.counting()));
        sb.append("\nTop source IPs:\n");
        topIps.entrySet().stream()
                .sorted(Map.Entry.<String, Long>comparingByValue().reversed())
                .limit(10)
                .forEach(en -> sb.append("  ").append(en.getKey()).append(": ").append(en.getValue()).append("\n"));
        return sb.toString();
    }

    public static void main(String[] args) throws Exception {
        Path logFile = Paths.get(args.length > 0 ? args[0] : "security_events.log");

        try (SecurityEventLogger logger = new SecurityEventLogger(logFile, 1000)) {
            logger.addRule(new ThresholdRule("BRUTE_FORCE_SUSPECTED", "LOGIN_FAILURE", 5, Duration.ofMinutes(1), Severity.HIGH));
            logger.addRule(new ThresholdRule("PORT_SCAN_SUSPECTED", "CONNECTION_REFUSED", 20, Duration.ofSeconds(30), Severity.MEDIUM));
            logger.addRule(new SeverityRule());

            ExecutorService pool = Executors.newFixedThreadPool(4);
            List<Future<?>> futures = new ArrayList<>();
            String[] ips = {"10.0.0.5", "192.168.1.20", "172.16.4.9"};
            String[] users = {"alice", "bob", "carol", "root"};

            for (int t = 0; t < 4; t++) {
                final int seed = t;
                futures.add(pool.submit(() -> {
                    Random rnd = new Random(seed);
                    for (int i = 0; i < 25; i++) {
                        String ip = ips[rnd.nextInt(ips.length)];
                        String user = users[rnd.nextInt(users.length)];
                        String type;
                        Severity sev;
                        switch (rnd.nextInt(5)) {
                            case 0 -> { type = "LOGIN_SUCCESS"; sev = Severity.INFO; }
                            case 1 -> { type = "LOGIN_FAILURE"; sev = Severity.LOW; }
                            case 2 -> { type = "CONNECTION_REFUSED"; sev = Severity.LOW; }
                            case 3 -> { type = "PRIVILEGE_CHANGE"; sev = Severity.MEDIUM; }
                            default -> { type = "MALWARE_DETECTED"; sev = Severity.CRITICAL; }
                        }
                        logger.log("SENSOR-" + seed, type, sev, user, ip, "Automated event " + i);
                        Thread.sleep(1);
                    }
                    return null;
                }));
            }

            for (Future<?> f : futures) f.get();
            pool.shutdown();

            System.out.println("Events logged: " + logger