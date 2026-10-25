#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define BUFFER_SIZE 4096
#define TOKEN_BYTES 32
#define TOKEN_HEX_LEN (TOKEN_BYTES * 2)
#define MAX_SESSIONS 1024
#define SESSION_TTL_SEC 300
#define USERNAME_MAX 64

typedef struct {
    char session_id[TOKEN_HEX_LEN + 1];
    char username[USERNAME_MAX];
    time_t expires_at;
    int is_active;
} Session;

typedef struct {
    Session sessions[MAX_SESSIONS];
    size_t count;
    pthread_mutex_t lock;
} SessionStore;

typedef struct {
    char method[16];
    char path[256];
    char cookie_token[TOKEN_HEX_LEN + 1];
    char body[BUFFER_SIZE];
} HttpRequest;

static SessionStore g_store;

static int secure_random_bytes(unsigned char *out, size_t length) {
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) {
        return -1;
    }
    size_t total_read = 0;
    while (total_read < length) {
        ssize_t n = read(fd, out + total_read, length - total_read);
        if (n <= 0) {
            close(fd);
            return -1;
        }
        total_read += (size_t)n;
    }
    close(fd);
    return 0;
}

static int generate_session_id(char *out_hex, size_t out_size) {
    if (out_size < TOKEN_HEX_LEN + 1) {
        return -1;
    }
    unsigned char random_bytes[TOKEN_BYTES];
    if (secure_random_bytes(random_bytes, TOKEN_BYTES) != 0) {
        return -1;
    }
    static const char hex_digits[] = "0123456789abcdef";
    for (size_t i = 0; i < TOKEN_BYTES; i++) {
        out_hex[i * 2]     = hex_digits[(random_bytes[i] >> 4) & 0x0F];
        out_hex[i * 2 + 1] = hex_digits[random_bytes[i] & 0x0F];
    }
    out_hex[TOKEN_HEX_LEN] = '\0';
    return 0;
}

static int constant_time_compare(const char *a, const char *b, size_t len) {
    unsigned char result = 0;
    for (size_t i = 0; i < len; i++) {
        result |= (unsigned char)(a[i] ^ b[i]);
    }
    return result == 0 ? 0 : -1;
}

static int init_session_store(SessionStore *store) {
    memset(store, 0, sizeof(SessionStore));
    if (pthread_mutex_init(&store->lock, NULL) != 0) {
        return -1;
    }
    return 0;
}

static void purge_expired_sessions_locked(SessionStore *store, time_t now) {
    for (size_t i = 0; i < MAX_SESSIONS; i++) {
        if (store->sessions[i].is_active && store->sessions[i].expires_at <= now) {
            store->sessions[i].is_active = 0;
            memset(store->sessions[i].session_id, 0, sizeof(store->sessions[i].session_id));
            memset(store->sessions[i].username, 0, sizeof(store->sessions[i].username));
            store->sessions[i].expires_at = 0;
            if (store->count > 0) {
                store->count--;
            }
        }
    }
}

static int create_session(SessionStore *store, const char *username, char *out_id, size_t out_id_size) {
    if (!store || !username || !out_id) {
        return -1;
    }
    char new_id[TOKEN_HEX_LEN + 1];
    if (generate_session_id(new_id, sizeof(new_id)) != 0) {
        return -1;
    }

    pthread_mutex_lock(&store->lock);
    time_t now = time(NULL);
    purge_expired_sessions_locked(store, now);

    int target_idx = -1;
    for (size_t i = 0; i < MAX_SESSIONS; i++) {
        if (!store->sessions[i].is_active) {
            target_idx = (int)i;
            break;
        }
    }

    if (target_idx == -1) {
        pthread_mutex_unlock(&store->lock);
        return -1;
    }

    strncpy(store->sessions[target_idx].session_id, new_id, sizeof(store->sessions[target_idx].session_id) - 1);
    strncpy(store->sessions[target_idx].username, username, sizeof(store->sessions[target_idx].username) - 1);
    store->sessions[target_idx].expires_at = now + SESSION_TTL_SEC;
    store->sessions[target_idx].is_active = 1;
    store->count++;

    pthread_mutex_unlock(&store->lock);

    snprintf(out_id, out_id_size, "%s", new_id);
    return 0;
}

static int validate_session(SessionStore *store, const char *token, char *out_user, size_t out_user_size) {
    if (!store || !token || strlen(token) != TOKEN_HEX_LEN) {
        return -1;
    }

    pthread_mutex_lock(&store->lock);
    time_t now = time(NULL);
    int found = -1;

    for (size_t i = 0; i < MAX_SESSIONS; i++) {
        if (store->sessions[i].is_active) {
            if (constant_time_compare(store->sessions[i].session_id, token, TOKEN_HEX_LEN) == 0) {
                if (store->sessions[i].expires_at > now) {
                    store->sessions[i].expires_at = now + SESSION_TTL_SEC;
                    if (out_user && out_user_size > 0) {
                        strncpy(out_user, store->sessions[i].username, out_user_size - 1);
                        out_user[out_user_size - 1] = '\0';
                    }
                    found = 0;
                } else {
                    store->sessions[i].is_active = 0;
                    memset(store->sessions[i].session_id, 0, sizeof(store->sessions[i].session_id));
                    if (store->count > 0) {
                        store->count--;
                    }
                }
                break;
            }
        }
    }

    pthread_mutex_unlock(&store->lock);
    return found;
}

static int terminate_session(SessionStore *store, const char *token) {
    if (!store || !token || strlen(token) != TOKEN_HEX_LEN) {
        return -1;
    }

    pthread_mutex_lock(&store->lock);
    int terminated = -1;

    for (size_t i = 0; i < MAX_SESSIONS; i++) {
        if (store->sessions[i].is_active) {
            if (constant_time_compare(store->sessions[i].session_id, token, TOKEN_HEX_LEN) == 0) {
                store->sessions[i].is_active = 0;
                memset(store->sessions[i].session_id, 0, sizeof(store->sessions[i].session_id));
                memset(store->sessions[i].username, 0, sizeof(store->sessions[i].username));
                store->sessions[i].expires_at = 0;
                if (store->count > 0) {
                    store->count--;
                }
                terminated = 0;
                break;
            }
        }
    }

    pthread_mutex_unlock(&store->lock);
    return terminated;
}

static int extract_cookie_value(const char *cookie_header, const char *key, char *out_val, size_t out_val_size) {
    const char *pos = cookie_header;
    size_t key_len = strlen(key);

    while ((pos = strstr(pos, key)) != NULL) {
        if ((pos == cookie_header || *(pos - 1) == ' ' || *(pos - 1) == ';') && *(pos + key_len) == '=') {
            pos += key_len + 1;
            size_t i = 0;
            while (*pos && *pos != ';' && *pos != ' ' && *pos != '\r' && *pos != '\n' && i < out_val_size - 1) {
                out_val[i++] = *pos++;
            }
            out_val[i] = '\0';
            return 0;
        }
        pos += key_len;
    }
    return -1;
}

static int parse_form_value(const char *body, const char *key, char *out_val, size_t out_val_size) {
    const char *pos = body;
    size_t key_len = strlen(key);

    while ((pos = strstr(pos, key)) != NULL) {
        if ((pos == body || *(pos - 1) == '&') && *(pos + key_len) == '=') {
            pos += key_len + 1;
            size_t i = 0;
            while (*pos && *pos != '&' && *pos != '\r' && *pos != '\n' && i < out_val_size - 1) {
                out_val[i++] = *pos++;
            }
            out_val[i] = '\0';
            return 0;
        }
        pos += key_len;
    }
    return -1;
}

static int parse_http_request(const char *raw, HttpRequest *req) {
    memset(req, 0, sizeof(HttpRequest));
    if (!raw) {
        return -1;
    }

    char line_buffer[BUFFER_SIZE];
    const char *line_end = strstr(raw, "\r\n");
    if (!line_end) {
        return -1;
    }

    size_t line_len = (size_t)(line_end - raw);
    if (line_len >= sizeof(line_buffer)) {
        return -1;
    }
    strncpy(line_buffer, raw, line_len);
    line_buffer[line_len] = '\0';

    if (sscanf(line_buffer, "%15s %255s", req->method, req->path) != 2) {
        return -1;
    }

    const char *header_start = line_end + 2;
    const char *body_separator = strstr(raw, "\r\n\r\n");
    if (!body_separator) {
        body_separator = strstr(raw, "\n\n");
    }

    if (body_separator) {
        const char *body_start = body_separator + (body_separator[0] == '\r' ? 4 : 2);
        strncpy(req->body, body_start, sizeof(req->body) - 1);
    }

    const