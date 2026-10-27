#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>

#define MAX_EVENTS 1000
#define LOG_FILE "security_events.log"
#define MAX_DESC 256
#define MAX_USER 64

typedef enum {
    EVENT_INFO = 0,
    EVENT_WARNING = 1,
    EVENT_CRITICAL = 2
} SeverityLevel;

typedef enum {
    EVENT_LOGIN_SUCCESS = 0,
    EVENT_LOGIN_FAILURE = 1,
    EVENT_FILE_ACCESS = 2,
    EVENT_PRIVILEGE_CHANGE = 3,
    EVENT_SYSTEM_START = 4,
    EVENT_SYSTEM_STOP = 5,
    EVENT_UNAUTHORIZED_ACCESS = 6,
    EVENT_CONFIG_CHANGE = 7
} EventType;

typedef struct {
    time_t timestamp;
    SeverityLevel severity;
    EventType type;
    char username[MAX_USER];
    char description[MAX_DESC];
    char source_ip[16];
} SecurityEvent;

typedef struct {
    SecurityEvent events[MAX_EVENTS];
    int count;
    int capacity;
} EventLog;

const char* severity_to_string(SeverityLevel sev) {
    switch(sev) {
        case EVENT_INFO: return "INFO";
        case EVENT_WARNING: return "WARNING";
        case EVENT_CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

const char* event_type_to_string(EventType type) {
    switch(type) {
        case EVENT_LOGIN_SUCCESS: return "LOGIN_SUCCESS";
        case EVENT_LOGIN_FAILURE: return "LOGIN_FAILURE";
        case EVENT_FILE_ACCESS: return "FILE_ACCESS";
        case EVENT_PRIVILEGE_CHANGE: return "PRIVILEGE_CHANGE";
        case EVENT_SYSTEM_START: return "SYSTEM_START";
        case EVENT_SYSTEM_STOP: return "SYSTEM_STOP";
        case EVENT_UNAUTHORIZED_ACCESS: return "UNAUTHORIZED_ACCESS";
        case EVENT_CONFIG_CHANGE: return "CONFIG_CHANGE";
        default: return "UNKNOWN";
    }
}

void get_timestamp(char* buffer, size_t size) {
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", tm_info);
}

EventLog* create_event_log(int capacity) {
    EventLog* log = (EventLog*)malloc(sizeof(EventLog));
    if (!log) return NULL;
    log->capacity = capacity;
    log->count = 0;
    return log;
}

int add_event(EventLog* log, SeverityLevel severity, EventType type,
              const char* username, const char* description, const char* source_ip) {
    if (!log || log->count >= log->capacity) return -1;

    SecurityEvent* event = &log->events[log->count];
    event->timestamp = time(NULL);
    event->severity = severity;
    event->type = type;

    strncpy(event->username, username ? username : "unknown", MAX_USER - 1);
    event->username[MAX_USER - 1] = '\0';

    strncpy(event->description, description ? description : "", MAX_DESC - 1);
    event->description[MAX_DESC - 1] = '\0';

    strncpy(event->source_ip, source_ip ? source_ip : "0.0.0.0", 15);
    event->source_ip[15] = '\0';

    log->count++;
    return 0;
}

int write_event_to_file(FILE* fp, const SecurityEvent* event) {
    if (!fp || !event) return -1;

    char time_buf[64];
    get_timestamp(time_buf, sizeof(time_buf));

    fprintf(fp, "[%s] [%s] [%s] User: %s | IP: %s | %s\n",
            time_buf,
            severity_to_string(event->severity),
            event_type_to_string(event->type),
            event->username,
            event->source_ip,
            event->description);

    return 0;
}

int log_event_to_file(const char* filename, const SecurityEvent* event) {
    FILE* fp = fopen(filename, "a");
    if (!fp) return -1;

    int result = write_event_to_file(fp, event);
    fclose(fp);
    return result;
}

int flush_log_to_file(EventLog* log, const char* filename) {
    if (!log || !filename) return -1;

    FILE* fp = fopen(filename, "w");
    if (!fp) return -1;

    for (int i = 0; i < log->count; i++) {
        write_event_to_file(fp, &log->events[i]);
    }

    fclose(fp);
    return log->count;
}

void display_event(const SecurityEvent* event) {
    if (!event) return;

    char time_buf[64];
    struct tm* tm_info = localtime(&event->timestamp);
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);

    printf("[%s] [%s] [%s] User: %s | IP: %s | %s\n",
           time_buf,
           severity_to_string(event->severity),
           event_type_to_string(event->type),
           event->username,
           event->source_ip,
           event->description);
}

void monitor_events(EventLog* log, SeverityLevel min_severity) {
    if (!log) return;

    printf("\n=== Security Event Monitor ===\n");
    printf("Showing events with severity >= %s\n\n", severity_to_string(min_severity));

    int displayed = 0;
    for (int i = 0; i < log->count; i++) {
        if (log->events[i].severity >= min_severity) {
            display_event(&log->events[i]);
            displayed++;
        }
    }

    if (displayed == 0) {
        printf("No events match the criteria.\n");
    }
    printf("\nTotal events displayed: %d\n", displayed);
}

int count_events_by_type(EventLog* log, EventType type) {
    if (!log) return 0;

    int count = 0;
    for (int i = 0; i < log->count; i++) {
        if (log->events[i].type == type) {
            count++;
        }
    }
    return count;
}

int count_events_by_severity(EventLog* log, SeverityLevel severity) {
    if (!log) return 0;

    int count = 0;
    for (int i = 0; i < log->count; i++) {
        if (log->events[i].severity == severity) {
            count++;
        }
    }
    return count;
}

void generate