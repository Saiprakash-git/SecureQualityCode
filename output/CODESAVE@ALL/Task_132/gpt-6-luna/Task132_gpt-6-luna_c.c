#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define MAX_TEXT 512
#define MAX_TRACKED_SOURCES 1024
#define MAX_FAILURES 64
#define INPUT_BUFFER_SIZE 4096
#define RECORD_BUFFER_SIZE 8192

typedef struct {
    int fd;
} Logger;

typedef struct {
    int used;
    char source[MAX_TEXT];
    time_t failures[MAX_FAILURES];
    size_t failure_count;
    int alerted;
    time_t last_seen;
} SourceState;

typedef struct {
    SourceState sources[MAX_TRACKED_SOURCES];
    uint64_t events_logged;
    uint64_t login_failures;
    uint64_t alerts_logged;
} Monitor;

static int write_all(int fd, const char *buffer, size_t length)
{
    size_t written = 0;

    while (written < length) {
        ssize_t result = write(fd, buffer + written, length - written);
        if (result < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (result == 0)
            return -1;
        written += (size_t)result;
    }

    return 0;
}

static int logger_open(Logger *logger, const char *path)
{
    struct stat status;
    static const char header[] = "timestamp_utc,event,user,source,detail\n";

    logger->fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0600);
    if (logger->fd < 0)
        return -1;

    if (flock(logger->fd, LOCK_EX) < 0) {
        close(logger->fd);
        logger->fd = -1;
        return -1;
    }

    if (fstat(logger->fd, &status) < 0 ||
        (status.st_size == 0 &&
         write_all(logger->fd, header, sizeof(header) - 1) < 0)) {
        int saved_errno = errno;
        flock(logger->fd, LOCK_UN);
        close(logger->fd);
        logger->fd = -1;
        errno = saved_errno;
        return -1;
    }

    if (flock(logger->fd, LOCK_UN) < 0) {
        close(logger->fd);
        logger->fd = -1;
        return -1;
    }

    return 0;
}

static void logger_close(Logger *logger)
{
    if (logger->fd >= 0) {
        close(logger->fd);
        logger->fd = -1;
    }
}

static int append_csv_field(char *buffer, size_t capacity, size_t *used,
                            const char *value)
{
    size_t i;

    if (*used + 1 >= capacity)
        return -1;
    buffer[(*used)++] = '"';

    for (i = 0; value[i] != '\0'; ++i) {
        unsigned char ch = (unsigned char)value[i];

        if (ch == '"') {
            if (*used + 2 >= capacity)
                return -1;
            buffer[(*used)++] = '"';
            buffer[(*used)++] = '"';
        } else {
            if (*used + 1 >= capacity)
                return -1;
            buffer[(*used)++] = (ch < 32 || ch == 127) ? ' ' : (char)ch;
        }
    }

    if (*used + 2 >= capacity)
        return -1;
    buffer[(*used)++] = '"';
    buffer[(*used)++] = ',';
    return 0;
}

static int logger_write_event(Logger *logger, const char *event,
                              const char *user, const char *source,
                              const char *detail)
{
    char timestamp[32];
    char record[RECORD_BUFFER_SIZE];
    struct tm utc;
    time_t now;
    size_t used = 0;
    int result;

    now = time(NULL);
    if (now == (time_t)-1 || gmtime_r(&now, &utc) == NULL)
        return -1;

    result = snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02dT%02d:%02d:%02dZ",
                      utc.tm_year + 1900, utc.tm_mon + 1, utc.tm_mday,
                      utc.tm_hour, utc.tm_min, utc.tm_sec);
    if (result < 0 || (size_t)result >= sizeof(timestamp))
        return -1;

    if (append_csv_field(record, sizeof(record), &used, timestamp) < 0 ||
        append_csv_field(record, sizeof(record), &used, event) < 0 ||
        append_csv_field(record, sizeof(record), &used, user) < 0 ||
        append_csv_field(record, sizeof(record), &used, source) < 0 ||
        append_csv_field(record, sizeof(record), &used, detail) < 0)
        return -1;

    if (used == 0 || used >= sizeof(record))
        return -1;
    record[used - 1] = '\n';

    if (flock(logger->fd, LOCK_EX) < 0)
        return -1;

    result = write_all(logger->fd, record, used);
    {
        int saved_errno = errno;
        if (flock(logger->fd, LOCK_UN) < 0 && result == 0)
            result = -1;
        errno = saved_errno;
    }

    return result;
}

static int record_event(Logger *logger, Monitor *monitor, const char *event,
                        const char *user, const char *source,
                        const char *detail)
{
    if (logger_write_event(logger, event, user, source, detail) < 0) {
        fprintf(stderr, "Could not write event to log: %s\n", strerror(errno));
        return -1;
    }

    monitor->events_logged++;
    if (strcmp(event, "LOGIN_FAILURE") == 0)
        monitor->login_failures++;
    if (strcmp(event, "BRUTE_FORCE_ALERT") == 0)
        monitor->alerts_logged++;

    return 0;
}

static SourceState *find_source_state(Monitor *monitor, const char *source,
                                      time_t now)
{
    SourceState *unused = NULL;
    size_t i;

    for (i = 0; i < MAX_TRACKED_SOURCES; ++i) {
        SourceState *state = &monitor->sources[i];

        if (state->used && strcmp(state->source, source) == 0)
            return state;
        if (!state->used && unused == NULL)
            unused = state;
    }

    if (unused == NULL)
        return NULL;

    unused->used = 1;
    snprintf(unused->source, sizeof(unused->source), "%s", source);
    unused->failure_count = 0;
    unused->alerted = 0;
    unused->last_seen = now;
    return unused;
}

static void monitor_login_failure(Logger *logger, Monitor *monitor,
                                  const char *user, const char *source,
                                  const char *detail, unsigned threshold,
                                  unsigned window_seconds)
{
    time_t now = time(NULL);
    SourceState *state;
    size_t first_valid = 0;
    size_t i;

    if (record_event(logger, monitor, "LOGIN_FAILURE", user, source, detail) < 0)
        return;

    if (now == (time_t)-1)
        return;

    state = find_source_state(monitor, source, now);
    if (state == NULL) {
        fprintf(stderr, "Source tracking capacity reached; failure was logged but not monitored.\n");
        return;
    }

    state->last_seen = now;

    while (first_valid < state->failure_count &&
           now - state->failures[first_valid] > (time_t)window_seconds)
        first_valid++;

    if (first_valid > 0) {
        for (i = first_valid; i < state->failure_count; ++i)
            state->failures[i - first_valid] = state->failures[i];
        state->failure_count -= first_valid;
    }

    if (state->failure_count < MAX_FAILURES)
        state->failures[state->failure_count++] = now;

    if (state->failure_count < threshold)
        state->alerted = 0;

    if (state->failure_count >= threshold && !state->alerted) {
        char alert_detail[MAX_TEXT];
        int length = snprintf(alert_detail, sizeof(alert_detail),
                              "%zu failed logins within %u seconds (threshold %u)",
                              state->failure_count, window_seconds, threshold);

        state->alerted = 1;
        if (length < 0)
            snprintf(alert_detail, sizeof(alert_detail), "Repeated login failures detected");
        else if ((size_t)length >= sizeof(alert_detail))
            alert_detail[sizeof(alert_detail) - 1] = '\0';

        if (record_event(logger, monitor, "BRUTE_FORCE_ALERT", "system",
                         source, alert_detail) == 0)
            fprintf(stderr, "SECURITY ALERT: %s from %s\n", alert_detail, source);
    }
}

static char *next_token(char **cursor)
{
    char *start = *cursor;

    while (*start != '\0' && isspace((unsigned char)*start))
        start++;

    if (*start == '\0') {
        *cursor = start;
        return NULL;
    }

    *cursor = start;
    while (**cursor != '\0' && !isspace((unsigned char)**cursor))
        (*cursor)++;

    if (**cursor != '\0') {
        **cursor = '\0';
        (*cursor)