#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#define MAX_EVENTS 1024
#define MAX_IP_TRACK 128
#define IP_STR_LEN 16
#define USER_STR_LEN 32
#define DESC_STR_LEN 128
#define BRUTE_FORCE_THRESHOLD 3
#define BRUTE_FORCE_WINDOW_SEC 60

typedef enum {
    SEV_INFO,
    SEV_LOW,
    SEV_MEDIUM,
    SEV_HIGH,
    SEV_CRITICAL
} SeverityLevel;

typedef enum {
    EVT_AUTH_SUCCESS,
    EVT_AUTH_FAILURE,
    EVT_FILE_ACCESS,
    EVT_PRIV_ESCALATION,
    EVT_PORT_SCAN
} EventType;

typedef struct {
    unsigned long event_id;
    time_t timestamp;
    EventType type;
    SeverityLevel severity;
    char user[USER_STR_LEN];
    char source_ip[IP_STR_LEN];
    char description[DESC_STR_LEN];
} SecurityEvent;

typedef struct {
    char ip[IP_STR_LEN];
    int failed_attempts;
    time_t first_failure_time;
    time_t last_failure_time;
    bool blocked;
} IPTracker;

typedef struct {
    SecurityEvent events[MAX_EVENTS];
    size_t event_count;
    unsigned long next_event_id;
    IPTracker tracked_ips[MAX_IP_TRACK];
    size_t tracker_count;
    size_t alerts_triggered;
    FILE *log_file;
} SecurityMonitor;

static const char *severity_to_string(SeverityLevel sev) {
    switch (sev) {
        case SEV_INFO:     return "INFO";
        case SEV_LOW:      return "LOW";
        case SEV_MEDIUM:   return "MEDIUM";
        case SEV_HIGH:     return "HIGH";
        case SEV_CRITICAL: return "CRITICAL";
        default:           return "UNKNOWN";
    }
}

static const char *event_type_to_string(EventType type) {
    switch (type) {
        case EVT_AUTH_SUCCESS:     return "AUTH_SUCCESS";
        case EVT_AUTH_FAILURE:     return "AUTH_FAILURE";
        case EVT_FILE_ACCESS:      return "FILE_ACCESS";
        case EVT_PRIV_ESCALATION:  return "PRIV_ESCALATION";
        case EVT_PORT_SCAN:        return "PORT_SCAN";
        default:                   return "UNKNOWN_EVENT";
    }
}

int monitor_init(SecurityMonitor *mon, const char *log_filepath) {
    if (!mon) return -1;

    memset(mon, 0, sizeof(SecurityMonitor));
    mon->next_event_id = 1;

    if (log_filepath) {
        mon->log_file = fopen(log_filepath, "a");
        if (!mon->log_file) {
            perror("Failed to open log file");
            return -1;
        }
    }
    return 0;
}

void monitor_destroy(SecurityMonitor *mon) {
    if (!mon) return;
    if (mon->log_file) {
        fclose(mon->log_file);
        mon->log_file = NULL;
    }
}

static IPTracker *find_or_create_tracker(SecurityMonitor *mon, const char *ip) {
    for (size_t i = 0; i < mon->tracker_count; ++i) {
        if (strncmp(mon->tracked_ips[i].ip, ip, IP_STR_LEN) == 0) {
            return &mon->tracked_ips[i];
        }
    }
    if (mon->tracker_count < MAX_IP_TRACK) {
        IPTracker *t = &mon->tracked_ips[mon->tracker_count++];
        strncpy(t->ip, ip, IP_STR_LEN - 1);
        t->ip[IP_STR_LEN - 1] = '\0';
        t->failed_attempts = 0;
        t->first_failure_time = 0;
        t->last_failure_time = 0;
        t->blocked = false;
        return t;
    }
    return NULL;
}

static void trigger_alert(SecurityMonitor *mon, SeverityLevel sev, const char *rule, const char *detail) {
    mon->alerts_triggered++;
    time_t now = time(NULL);
    char time_buf[26];
    ctime_r(&now, time_buf);
    time_buf[24] = '\0';

    printf("[ALERT] [%s] [%s] Rule: %s - %s\n", time_buf, severity_to_string(sev), rule, detail);

    if (mon->log_file) {
        fprintf(mon->log_file, "[ALERT] [%s] [%s] Rule: %s - %s\n", time_buf, severity_to_string(sev), rule, detail);
        fflush(mon->log_file);
    }
}

static void analyze_event(SecurityMonitor *mon, const SecurityEvent *ev) {
    if (ev->type == EVT_AUTH_FAILURE) {
        IPTracker *t = find_or_create_tracker(mon, ev->source_ip);
        if (t) {
            if (t->blocked) {
                trigger_alert(mon, SEV_HIGH, "BLOCKED_IP_ACTIVITY", ev->source_ip);
                return;
            }

            if (t->failed_attempts == 0 || (ev->timestamp - t->first_failure_time) > BRUTE_FORCE_WINDOW_SEC) {
                t->failed_attempts = 1;
                t->first_failure_time = ev->timestamp;
            } else {
                t->failed_attempts++;
            }
            t->last_failure_time = ev->timestamp;

            if (t->failed_attempts >= BRUTE_FORCE_THRESHOLD) {
                t->blocked = true;
                char alert_msg[128];
                snprintf(alert_msg, sizeof(alert_msg), "Brute force detected from %s (%d failures). IP blocked.",
                         t->ip, t->failed_attempts);
                trigger_alert(mon, SEV_CRITICAL, "BRUTE_FORCE_DETECTED", alert_msg);
            }
        }
    } else if (ev->type == EVT_AUTH_SUCCESS) {
        IPTracker *t = find_or_create_tracker(mon, ev->source_ip);
        if (t && !t->blocked) {
            t->failed_attempts = 0;
            t->first_failure_time = 0;
        }
    } else if (ev->type == EVT_PRIV_ESCALATION) {
        if (strcmp(ev->user, "root") != 0 && strcmp(ev->user, "admin") != 0) {
            char alert_msg[128];
            snprintf(alert_msg, sizeof(alert_msg), "Suspicious privilege escalation attempt by user '%s'", ev->user);
            trigger_alert(mon, SEV_CRITICAL, "UNAUTHORIZED_ESCALATION", alert_msg);
        }
    } else if (ev->type == EVT_FILE_ACCESS) {
        if (strstr(ev->description, "/etc/shadow") || strstr(ev->description, "/etc/passwd")) {
            char alert_msg[128];
            snprintf(alert_msg, sizeof(alert_msg), "Sensitive file access by user '%s': %s", ev->user, ev->description);
            trigger_alert(mon, SEV_HIGH, "SENSITIVE_FILE_ACCESS", alert_msg);
        }
    } else if (ev->type == EVT_PORT_SCAN) {
        char alert_msg[128];
        snprintf(alert_msg, sizeof(alert_msg), "Reconnaissance scan detected from source %s", ev->source_ip);
        trigger_alert(mon, SEV_MEDIUM, "PORT_SCAN_DETECTED", alert_msg);
    }
}

int record_event(SecurityMonitor *mon,
                 EventType type,
                 SeverityLevel sev,
                 const char *user,
                 const char *source_ip,
                 const char *desc) {
    if (!mon || !user || !source_ip || !desc) return -1;
    if (mon->event_count >= MAX_EVENTS) return -2;

    SecurityEvent *ev = &mon->events[mon->event_count++];
    ev->event_id = mon->next_event_id++;
    ev->timestamp = time(NULL);
    ev->type = type;
    ev->severity = sev;

    strncpy(ev->user, user, USER_STR_LEN - 1);
    ev->user[USER_STR_LEN - 1] = '\0';

    strncpy(ev->source_ip, source_ip, IP_STR_LEN - 1);
    ev->source_ip[IP_STR_LEN - 1] = '\0';

    strncpy(ev->description, desc, DESC_STR_LEN - 1);
    ev->description[DESC_STR_LEN - 1] = '\0';

    char time_buf[26];
    ctime_r(&ev->timestamp, time_buf);
    time_buf[24] = '\0';

    printf("[LOG] #%lu [%s] [%s] [%s] user=%s ip=%s desc=\"%s\"\n",
           ev->event_id,
           time_buf,
           severity_to_string(ev->severity),
           event_type_to_string(ev->type),
           ev->user,
           ev->source_ip,
           ev->description);

    if (mon->log_file) {
        fprintf(mon->log_file, "[LOG] #%lu [%s] [%s] [%s] user=%s ip=%s desc=\"%s\"\n",
                ev->event_id,
                time_buf,
                severity_to_string(ev->severity),
                event_type_to_string(ev->type),
                ev->user,
                ev->source_ip,
                ev->description);
        fflush(mon->log_file);
    }

    analyze_event(mon, ev);
    return 0;
}

void print_monitor_report(const SecurityMonitor *mon) {
    if (!mon) return;

    printf("\n=================== SECURITY MONITOR REPORT ===================\n");
    printf("Total Events Logged:     %zu\n", mon->event_count);
    printf("Total Alerts Triggered:  %zu\n", mon->alerts_triggered);
    printf("Tracked Source IPs:      %zu\n", mon->tracker_count);
    printf("\nIP Tracking Status:\n");
    for (size_t i = 0; i < mon->tracker_count; ++i) {
        const IPTracker *t = &mon->tracked_ips[i];
        printf("  IP: %-15s | Failed: %2d | Blocked: %s\n",
               t->ip,
               t->failed_attempts,
               t->blocked ? "YES" : "NO");
    }
    printf("===============================================================\n\n");
}

int main(void) {
    SecurityMonitor monitor;
    if (monitor_init(&monitor, "security_events.log") != 0) {
        fprintf(stderr, "Failed to initialize security monitor\n");
        return EXIT_FAILURE;
    }

    printf("Starting security event monitoring...\n\n");

    record_event(&monitor, EVT_AUTH_SUCCESS, SEV_INFO, "alice", "192.168.1.10", "User logged in successfully via SSH");
    record_event(&monitor, EVT_AUTH_FAILURE, SEV_LOW, "unknown", "10.0.0.99", "Failed password attempt");
    record_event(&monitor, EVT_AUTH_FAILURE, SEV_LOW, "root", "10.0.0.99", "Failed password attempt");
    record_event(&monitor, EVT_AUTH_FAILURE, SEV_LOW, "admin", "10.0.0.99", "Failed password attempt");
    record_event(&monitor, EVT_AUTH_FAILURE, SEV_LOW, "support", "10.0.0.99", "Failed password attempt from banned IP");
    record_event(&monitor, EVT_FILE_ACCESS, SEV_MEDIUM, "bob", "192.168.1.20", "Accessed /var/log/syslog");
    record_event(&monitor, EVT_FILE_ACCESS, SEV_HIGH, "bob", "192.168.1.20", "Attempted read on /etc/shadow");
    record_event(&monitor, EVT_PRIV_ESCALATION, SEV_HIGH, "charlie", "192.168.1.30", "Sudo privileges acquired");
    record_event(&monitor, EVT_