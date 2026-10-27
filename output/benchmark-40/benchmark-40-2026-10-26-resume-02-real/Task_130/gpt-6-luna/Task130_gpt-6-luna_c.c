#define _POSIX_C_SOURCE 200809L

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define REQUEST_BUFFER_SIZE 8192
#define RESPONSE_BUFFER_SIZE 4096
#define DEFAULT_PORT 8080

static int send_all(int socket_fd, const char *data, size_t length)
{
    size_t sent = 0;

    while (sent < length) {
        ssize_t result = send(socket_fd, data + sent, length - sent, 0);
        if (result < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (result == 0) {
            return -1;
        }
        sent += (size_t)result;
    }

    return 0;
}

static int send_response(int socket_fd, int status, const char *reason,
                         const char *headers, const char *body)
{
    char response[RESPONSE_BUFFER_SIZE];
    size_t body_length = strlen(body);
    int response_length = snprintf(
        response, sizeof(response),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: application/json; charset=utf-8\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "%s"
        "\r\n"
        "%s",
        status, reason, body_length, headers, body);

    if (response_length < 0 || (size_t)response_length >= sizeof(response)) {
        return -1;
    }

    return send_all(socket_fd, response, (size_t)response_length);
}

static int origin_is_allowed(const char *origin)
{
    static const char *allowed_origins[] = {
        "http://localhost:3000",
        "https://app.example.com"
    };
    size_t i;

    if (origin == NULL || origin[0] == '\0') {
        return 0;
    }

    for (i = 0; i < sizeof(allowed_origins) / sizeof(allowed_origins[0]); i++) {
        if (strcmp(origin, allowed_origins[i]) == 0) {
            return 1;
        }
    }

    return 0;
}

static void make_cors_headers(char *output, size_t output_size,
                              const char *origin, int preflight)
{
    int length;

    if (origin_is_allowed(origin)) {
        length = snprintf(output, output_size,
                          "Vary: Origin\r\n"
                          "Access-Control-Allow-Origin: %s\r\n",
                          origin);
    } else {
        length = snprintf(output, output_size, "Vary: Origin\r\n");
    }

    if (length < 0 || (size_t)length >= output_size) {
        output[0] = '\0';
        return;
    }

    if (preflight) {
        size_t used = (size_t)length;
        length = snprintf(output + used, output_size - used,
                          "Access-Control-Allow-Methods: GET, OPTIONS\r\n"
                          "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
                          "Access-Control-Max-Age: 600\r\n");
        if (length < 0 || (size_t)length >= output_size - used) {
            output[0] = '\0';
        }
    }
}

static char *trim_whitespace(char *text)
{
    char *end;

    while (*text != '\0' && isspace((unsigned char)*text)) {
        text++;
    }

    end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1])) {
        end--;
    }
    *end = '\0';

    return text;
}

static int requested_headers_are_allowed(const char *headers)
{
    const char *cursor = headers;

    if (headers[0] == '\0') {
        return 1;
    }

    while (*cursor != '\0') {
        const char *comma = strchr(cursor, ',');
        size_t length = comma != NULL ? (size_t)(comma - cursor) : strlen(cursor);
        char name[128];
        size_t start = 0;
        size_t end = length;

        while (start < end && isspace((unsigned char)cursor[start])) {
            start++;
        }
        while (end > start && isspace((unsigned char)cursor[end - 1])) {
            end--;
        }

        if (start == end || end - start >= sizeof(name)) {
            return 0;
        }

        memcpy(name, cursor + start, end - start);
        name[end - start] = '\0';

        if (strcasecmp(name, "Content-Type") != 0 &&
            strcasecmp(name, "Authorization") != 0) {
            return 0;
        }

        if (comma == NULL) {
            return 1;
        }

        cursor = comma + 1;
        if (*cursor == '\0') {
            return 0;
        }
    }

    return 1;
}

static int copy_header_value(char *destination, size_t destination_size,
                             const char *value)
{
    size_t length = strlen(value);

    if (length >= destination_size) {
        return -1;
    }

    memcpy(destination, value, length + 1);
    return 0;
}

static void handle_client(int client_fd)
{
    char request[REQUEST_BUFFER_SIZE];
    char method[16] = "";
    char target[2048] = "";
    char version[16] = "";
    char origin[512] = "";
    char requested_method[32] = "";
    char requested_headers[512] = "";
    char cors_headers[1024];
    char *header_end;
    char *save_pointer = NULL;
    char *line;
    int origin_seen = 0;
    int requested_method_seen = 0;
    int requested_headers_seen = 0;
    ssize_t received = 0;
    int parsed;

    while ((size_t)received < sizeof(request) - 1) {
        ssize_t result = recv(client_fd, request + received,
                              sizeof(request) - 1 - (size_t)received, 0);
        if (result < 0) {
            if (errno == EINTR) {
                continue;
            }
            return;
        }
        if (result == 0) {
            return;
        }

        received += result;
        request[received] = '\0';

        if (strstr(request, "\r\n\r\n") != NULL) {
            break;
        }
    }

    header_end = strstr(request, "\r\n\r\n");
    if (header_end == NULL) {
        send_response(client_fd, 431, "Request Header Fields Too Large",
                      "Vary: Origin\r\n", "{\"error\":\"headers too large\"}");
        return;
    }

    if (memchr(request, '\0', (size_t)received) != NULL) {
        send_response(client_fd, 400, "Bad Request",
                      "Vary: Origin\r\n", "{\"error\":\"invalid request\"}");
        return;
    }

    *header_end = '\0';
    line = strtok_r(request, "\r\n", &save_pointer);
    if (line == NULL) {
        send_response(client_fd, 400, "Bad Request",
                      "Vary: Origin\r\n", "{\"error\":\"invalid request\"}");
        return;
    }

    parsed = sscanf(line, "%15s %2047s %15s", method, target, version);
    if (parsed != 3 ||
        (strcmp(version, "HTTP/1.1") != 0 && strcmp(version, "HTTP/1.0") != 0)) {
        send_response(client_fd, 400, "Bad Request",
                      "Vary: Origin\r\n", "{\"error\":\"invalid request\"}");
        return;
    }

    while ((line = strtok_r(NULL, "\r\n", &save_pointer)) != NULL) {
        char *colon = strchr(line, ':');
        char *name;
        char *value;

        if (colon == NULL) {
            send_response(client_fd, 400, "Bad Request",
                          "Vary: Origin\r\n", "{\"error\":\"invalid header\"}");
            return;
        }

        *colon = '\0';
        name = trim_whitespace(line);
        value = trim_whitespace(colon + 1);

        if (strcasecmp(name, "Origin") == 0) {
            if (origin_seen ||
                copy_header_value(origin, sizeof(origin), value) != 0) {
                send_response(client_fd, 400, "Bad Request",
                              "Vary: Origin\r\n", "{\"error\":\"invalid origin header\"}");
                return;
            }
            origin_seen = 1;
        } else if (strcasecmp(name, "Access-Control-Request-Method") == 0) {
            if (requested_method_seen ||
                copy_header_value(request