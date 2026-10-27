#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_LINE 1024
#define MAX_TRACK 256
#define BRUTE_THRESHOLD 5
#define BRUTE_WINDOW_SEC 60

typedef enum { SEV_INFO = 0, SEV_WARN = 1, SEV_CRIT = 2 } Severity;

typedef struct {
    time_t ts;
    Severity sev;
    char type[32];
    char source[64];
    char message[512];
} Event;

typedef struct {
    char source[64];
    time_t window_start;
    int count;
    int used;
} Tracker;

typedef struct {
    size_t total;
    size_t parsed_ok;
    size_t parse_errors;
    size_t alerts;
} Stats;

static const char *sev_name(Severity s) {
    switch (s) {
        case SEV_INFO: return "INFO";
        case SEV_WARN: return "WARN";
        case SEV_CRIT: return "CRIT";
    }
    return "UNKNOWN";
}

static int sev_parse(const char *s, Severity *out) {
    if (strcmp(s, "info") == 0 || strcmp(s, "INFO") == 0) { *out = SEV_INFO; return 1; }
    if (strcmp(s, "warn") == 0 || strcmp(s, "WARN") == 0) { *out = SEV_WARN; return 1; }
    if (strcmp(s, "crit") == 0 || strcmp(s, "CRIT") == 0) { *out = SEV_CRIT; return 1; }
    return 0;
}

int parse_event(const char *line, Event *e) {
    char sevbuf[16] = {0};
    long ts = 0;
    int n;

    if (!line || !e) return 0;
    memset(e, 0, sizeof(*e));

    n = sscanf(line, "%ld %15s %31s %63s %511[^\n]",
               &ts, sevbuf, e->type, e->source, e->message);
    if (n < 4) return 0;
    if (n == 4) e->message[0] = '\0';
    if (!sev_parse(sevbuf, &e->sev)) return 0;

    e->ts = (time_t)ts;
    return 1;
}

void format_event(const Event *e, char *buf, size_t size) {
    char tbuf[32];
    struct tm *tm_info;

    tm_info = gmtime(&e->ts);
    if (tm_info)
        strftime(tbuf, sizeof(tbuf), "%Y-%m-%dT%H:%M:%SZ", tm_info);
    else
        snprintf(tbuf, sizeof(tbuf), "invalid-time");

    snprintf(buf, size, "[%s] %-4s type=%s src=%s msg=\"%s\"",
             tbuf, sev_name(e->sev), e->type, e->source, e->message);
}

int log_event(FILE *out, const Event *e) {
    char line[MAX_LINE];
    if (!out || !e) return 0;
    format_event(e, line, sizeof(line));
    if (fprintf(out, "%s\n", line) < 0) return 0;
    fflush(out);
    return 1;
}

static Tracker *find_tracker(Tracker trackers[], size_t n, const char *source) {
    size_t i;
    for (i = 0; i < n; i++) {
        if (trackers[i].used && strcmp(trackers[i].source, source) == 0)
            return &trackers[i];
    }
    return NULL;
}

static Tracker *alloc_tracker(Tracker trackers[], size_t n, const char *source) {
    size_t i;
    for (i = 0; i < n; i++) {
        if (!trackers[i].used) {
            trackers[i].used = 1;
            trackers[i].count = 0;
            trackers[i].window_start = 0;
            strncpy(trackers[i].source, source, sizeof(trackers[i].source) - 1);
            trackers[i].source[sizeof(trackers[i].source) - 1] = '\0';
            return &trackers[i];
        }
    }
    return NULL;
}

/* Returns 1 if the event triggers a brute-force alert, 0 otherwise. */
int track_auth_failure(Tracker trackers[], size_t n, const Event *e) {
    Tracker *t;

    if (!trackers || !e) return 0;
    if (strcmp(e->type, "auth_fail") != 0) return 0;

    t = find_tracker(trackers, n, e->source);
    if (!t) t = alloc_tracker(trackers, n, e->source);
    if (!t) return 0;

    if (t->count == 0 || e->ts - t->window_start > BRUTE_WINDOW_SEC) {
        t->window_start = e->ts;
        t->count = 0;
    }
    t->count++;

    if (t->count == BRUTE_THRESHOLD) {
        return 1;
    }
    return 0;
}

/* Returns 1 if the event should raise an alert under the general rules. */
int needs_alert(const Event *e) {
    if (!e) return 0;
    if (e->sev == SEV_CRIT) return 1;
    return 0;
}

void process_stream(FILE *in, FILE *logf, FILE *alertf, Stats *stats,
                    Tracker trackers[], size_t ntrackers) {
    char line[MAX_LINE];
    Event e;
    char alert_buf[MAX_LINE + 64];

    while (fgets(line, sizeof(line), in)) {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';
        if (line[0] == '\0' || line[0] == '#') continue;

        stats->total++;

        if (!parse_event(line, &e)) {
            stats->parse_errors++;
            continue;
        }
        stats->parsed_ok++;

        log_event(logf, &e);

        if (needs_alert(&e)) {
            format_event(&e, alert_buf, sizeof(alert_buf));
            fprintf(alertf, "ALERT: critical event: %s\n", alert_buf);
            stats->alerts++;
        }

        if (track_auth_failure(trackers, ntrackers, &e)) {
            fprintf(alertf, "ALERT: possible brute force from %s (>= %d failures within %d s)\n",
                    e.source, BRUTE_THRESHOLD, BRUTE_WINDOW_SEC);
            stats->alerts++;
        }
    }
}

int main(int argc, char *argv[]) {
    FILE *in = stdin;
    FILE *logf = stdout;
    FILE *alertf = stderr;
    Tracker trackers[MAX_TRACK];
    Stats stats = {0, 0, 0, 0};
    int rc = EXIT_SUCCESS;

    memset(trackers, 0, sizeof(trackers));

    if (argc >= 2) {
        in = fopen(argv[1], "r");
        if (!in) {
            perror("open input");
            return EXIT_FAILURE;
        }
    }
    if (argc >= 3) {
        logf = fopen(argv[2], "a");
        if (!logf) {
            perror("open log");
            if (in != stdin) fclose(in);
            return EXIT_FAILURE;
        }
    }

    process_stream(in, logf, alertf, &stats, trackers, MAX_TRACK);

    fprintf(alertf, "Summary: lines=%zu parsed=%zu errors=%zu alerts=%zu\n",
            stats.total, stats.parsed_ok, stats.parse_errors, stats.alerts);

    if (stats.alerts > 0) rc = 2;

    if (in != stdin) fclose(in);
    if (logf != stdout) fclose(logf);
    return rc;
}