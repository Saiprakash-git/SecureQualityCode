#define _POSIX_C_SOURCE 200809L

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define SESSION_TTL 1800
#define SESSION_ID_HEX 64
#define CSRF_TOKEN_HEX 64
#define MAX_SESSION_DATA 4000
#define MAX_REQUEST_BODY 8192
#define MAX_HEADER_SIZE 8192
#define REQUEST_BUFFER_SIZE 32768
#define RESPONSE_BUFFER_SIZE 24000

typedef struct Session {
    char id[SESSION_ID_HEX + 1];
    char csrf[CSRF_TOKEN_HEX + 1];
    char data[MAX_SESSION_DATA + 1];
    time_t expires_at;
    struct Session *next;
} Session;

typedef struct {
    char method[16];
    char path[2048];
    char cookie[4096];
    char content_type[256];
    size_t content_length;
    char *body;
} HttpRequest;

static Session *sessions = NULL;
static pthread_mutex_t sessions_mutex = PTHREAD_MUTEX_INITIALIZER;

static int send_all(int fd, const void *buffer, size_t length)
{
    const char *p = (const char *)buffer;
    size_t sent = 0;

    while (sent < length) {
        ssize_t n = send(fd, p + sent, length - sent, 0);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (n == 0)
            return -1;
        sent += (size_t)n;
    }
    return 0;
}

static int fill_random_bytes(unsigned char *buffer, size_t length)
{
    int fd = open("/dev/urandom", O_RDONLY);
    size_t received = 0;

    if (fd < 0)
        return -1;

    while (received < length) {
        ssize_t n = read(fd, buffer + received, length - received);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            close(fd);
            return -1;
        }
        if (n == 0) {
            close(fd);
            return -1;
        }
        received += (size_t)n;
    }

    close(fd);
    return 0;
}

static int random_hex(char *output, size_t hex_length)
{
    static const char digits[] = "0123456789abcdef";
    unsigned char bytes[(SESSION_ID_HEX + 1) / 2];
    size_t byte_count = (hex_length + 1) / 2;
    size_t i;

    if (hex_length == 0 || hex_length > SESSION_ID_HEX ||
        fill_random_bytes(bytes, byte_count) != 0)
        return -1;

    for (i = 0; i < hex_length; i++) {
        unsigned char value = bytes[i / 2];
        output[i] = digits[(i & 1) ? (value & 0x0f) : (value >> 4)];
    }
    output[hex_length] = '\0';
    return 0;
}

static int is_hex_string(const char *value, size_t expected_length)
{
    size_t i;

    if (strlen(value) != expected_length)
        return 0;

    for (i = 0; i < expected_length; i++) {
        if (!((value[i] >= '0' && value[i] <= '9') ||
              (value[i] >= 'a' && value[i] <= 'f')))
            return 0;
    }
    return 1;
}

static void remove_expired_sessions_locked(time_t now)
{
    Session **link = &sessions;

    while (*link != NULL) {
        Session *current = *link;
        if (current->expires_at <= now) {
            *link = current->next;
            memset(current, 0, sizeof(*current));
            free(current);
        } else {
            link = &current->next;
        }
    }
}

static void delete_session_locked(const char *id)
{
    Session **link = &sessions;

    while (*link != NULL) {
        Session *current = *link;
        if (strcmp(current->id, id) == 0) {
            *link = current->next;
            memset(current, 0, sizeof(*current));
            free(current);
            return;
        }
        link = &current->next;
    }
}

static Session *find_session_locked(const char *id)
{
    Session *current = sessions;

    while (current != NULL) {
        if (strcmp(current->id, id) == 0)
            return current;
        current = current->next;
    }
    return NULL;
}

static int extract_session_cookie(const char *header, char output[SESSION_ID_HEX + 1])
{
    const char *p;
    int found = 0;

    output[0] = '\0';
    if (header == NULL || *header == '\0')
        return 0;

    p = header;
    while (*p != '\0') {
        const char *start;
        const char *end;
        const char *equals;
        size_t name_length;
        size_t value_length;

        while (*p == ';' || *p == ' ' || *p == '\t')
            p++;

        start = p;
        while (*p != '\0' && *p != ';')
            p++;
        end = p;

        while (end > start && (end[-1] == ' ' || end[-1] == '\t'))
            end--;

        equals = memchr(start, '=', (size_t)(end - start));
        if (equals == NULL)
            continue;

        name_length = (size_t)(equals - start);
        value_length = (size_t)(end - equals - 1);

        if (name_length == 3 && memcmp(start, "SID", 3) == 0) {
            if (found || value_length != SESSION_ID_HEX)
                return 0;
            memcpy(output, equals + 1, SESSION_ID_HEX);
            output[SESSION_ID_HEX] = '\0';
            if (!is_hex_string(output, SESSION_ID_HEX))
                return 0;
            found = 1;
        }
    }

    return found;
}

static int constant_time_equal(const char *a, const char *b, size_t length)
{
    unsigned char difference = 0;
    size_t i;

    for (i = 0; i < length; i++)
        difference |= (unsigned char)a[i] ^ (unsigned char)b[i];

    return difference == 0;
}

static int copy_active_session(const char *id, char csrf[CSRF_TOKEN_HEX + 1],
                               char data[MAX_SESSION_DATA + 1])
{
    Session *session;
    time_t now = time(NULL);
    int found = 0;

    pthread_mutex_lock(&sessions_mutex);
    remove_expired_sessions_locked(now);
    session = find_session_locked(id);

    if (session != NULL) {
        session->expires_at = now + SESSION_TTL;
        memcpy(csrf, session->csrf, CSRF_TOKEN_HEX + 1);
        memcpy(data, session->data, MAX_SESSION_DATA + 1);