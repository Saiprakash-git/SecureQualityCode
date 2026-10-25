#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 8080
#define MAX_REQUEST 65536
#define MAX_FIELD 4096

static int send_all(int fd, const char *data, size_t length)
{
    size_t sent = 0;

    while (sent < length) {
        ssize_t n = send(fd, data + sent, length - sent, 0);
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

static void send_response(int fd, const char *status, const char *body)
{
    char header[256];
    size_t body_length = strlen(body);
    int header_length = snprintf(header, sizeof(header),
                                 "HTTP/1.1 %s\r\n"
                                 "Content-Type: text/html; charset=utf-8\r\n"
                                 "Content-Length: %zu\r\n"
                                 "Connection: close\r\n"
                                 "X-Content-Type-Options: nosniff\r\n"
                                 "\r\n",
                                 status, body_length);

    if (header_length > 0 && (size_t)header_length < sizeof(header)) {
        send_all(fd, header, (size_t)header_length);
        send_all(fd, body, body_length);
    }
}

static void send_error(int fd, const char *status, const char *message)
{
    char body[512];

    snprintf(body, sizeof(body),
             "<!doctype html><html><head><meta charset=\"utf-8\">"
             "<title>Error</title></head><body><h1>%s</h1>"
             "<p>%s</p><p><a href=\"/\">Return to the form</a></p>"
             "</body></html>",
             status, message);
    send_response(fd, status, body);
}

static size_t html_escape(const char *input, char *output, size_t capacity)
{
    size_t used = 0;

    for (size_t i = 0; input[i] != '\0'; i++) {
        const char *replacement = NULL;

        switch (input[i]) {
        case '&':
            replacement = "&amp;";
            break;
        case '<':
            replacement = "&lt;";
            break;
        case '>':
            replacement = "&gt;";
            break;
        case '"':
            replacement = "&quot;";
            break;
        case '\'':
            replacement = "&#39;";
            break;
        default:
            break;
        }

        if (replacement != NULL) {
            size_t length = strlen(replacement);
            if (used + length >= capacity)
                break;
            memcpy(output + used, replacement, length);
            used += length;
        } else {
            if (used + 1 >= capacity)
                break;
            output[used++] = input[i];
        }
    }

    if (capacity > 0)
        output[used] = '\0';
    return used;
}

static void render_page(int fd, const char *message)
{
    char escaped[MAX_FIELD * 6 + 1];
    char body[MAX_FIELD * 7 + 2048];

    html_escape(message, escaped, sizeof(escaped));
    snprintf(body, sizeof(body),
             "<!doctype html>"
             "<html><head><meta charset=\"utf-8\">"
             "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
             "<title>Input Echo</title></head>"
             "<body><main><h1>Input Echo</h1>"
             "<form action=\"/\" method=\"post\">"
             "<label for=\"message\">Enter some text:</label><br>"
             "<textarea id=\"message\" name=\"message\" rows=\"5\" "
             "cols=\"50\" maxlength=\"4095\">%s</textarea><br>"
             "<button type=\"submit\">Submit</button></form>",
             escaped);

    if (message[0] != '\0') {
        size_t used = strlen(body);
        snprintf(body + used, sizeof(body) - used,
                 "<section><h2>Your input</h2><p>%s</p></section>",
                 escaped);
    }

    {
        size_t used = strlen(body);
        snprintf(body + used, sizeof(body) - used,
                 "</main></body></html>");
    }

    send_response(fd, "200 OK", body);
}

static int find_header_end(const char *request, size_t length, size_t *end)
{
    if (length < 4)
        return 0;

    for (size_t i = 0; i + 3 < length; i++) {
        if (request[i] == '\r' && request[i + 1] == '\n' &&
            request[i + 2] == '\r' && request[i + 3] == '\n') {
            *end = i + 4;
            return 1;
        }
    }
    return 0;
}

static int parse_content_length(const char *request, size_t header_end,
                                size_t *content_length)
{
    const char *line = strstr(request, "\r\n");

    if (line == NULL)
        return -1;
    line += 2;

    while ((size_t)(line - request) < header_end - 2) {
        const char *line_end = strstr(line, "\r\n");
        const char *colon;
        size_t name_length;

        if (line_end == NULL || line_end == line)
            break;

        colon = memchr(line, ':', (size_t)(line_end - line));
        if (colon == NULL)
            return -1;

        name_length = (size_t)(colon - line);
        if (name_length == 14 && strncasecmp(line, "Content-Length", 14) == 0) {
            const char *value = colon + 1;
            unsigned long long parsed = 0;

            while (value < line_end && isspace((unsigned char)*value))
                value++;
            if (value == line_end)
                return -1;

            for (; value < line_end; value++) {
                if (!isdigit((unsigned char)*value))
                    return -1;
                parsed = parsed * 10 + (unsigned long long)(*value - '0');
                if (parsed > MAX_REQUEST)
                    return -2;
            }

            *content_length = (size_t)parsed;
        }

        line = line_end + 2;
    }

    return 0;
}

static size_t decode_component(const char *input, size_t length,
                               char *output, size_t capacity)
{
    size_t used = 0;

    for (size_t i = 0; i < length && used + 1 < capacity; i++) {
        unsigned char value = (unsigned char)input[i];

        if (value == '+') {
            value = ' ';
        } else if (value == '%' && i + 2 < length &&
                   isxdigit((unsigned char)input[i + 1]) &&
                   isxdigit((unsigned char)input[i + 2])) {
            int high = isdigit((unsigned char)input[i + 1])
                           ? input[i + 1] - '0'
                           : tolower((unsigned char)input[i + 1]) - 'a' + 10;
            int low = isdigit((unsigned char)input[i + 2])
                          ? input[i + 2] - '0'
                          : tolower((unsigned char)input[i + 2]) - 'a' + 10;
            value = (unsigned char)((high << 4) | low);
            i += 2;
        }

        output[used++] = value == '\0' ? '?' : (char)value;
    }

    if (capacity > 0)
        output[used] = '\0';
    return used;
}

static void extract_message(const char *body, size_t body_length,
                            char *message, size_t message_capacity)
{
    size_t position = 0;

    message[0] = '\0';

    while (position < body_length) {
        size_t pair_end = position;
        size_t equals;
        char key[64];

        while (pair_end < body_length && body[pair_end] != '&')
            pair_end++;

        equals = position;
        while (equals < pair_end && body[equals] != '=')
            equals++;

        decode_component(body + position, equals - position,
                         key, sizeof(key));

        if (strcmp(key, "message") == 0) {
            size_t value_start = equals < pair_end ? equals + 1 : pair_end;
            decode_component(body + value_start, pair_end - value_start,
                             message, message_capacity);
            return;
        }

        position = pair_end + 1;
    }
}

static void handle_client(int fd)
{
    char request[MAX_REQUEST + 1];
    size_t received = 0;
    size_t header_end = 0;
    size_t content_length = 0;
    int header_status;
    char method[16] = {0};
    char path[1024] = {0};
    char message[MAX_FIELD] = {0};

    while (received < MAX_REQUEST) {
        ssize_t n = recv(fd, request + received, MAX_REQUEST - received, 0);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            return;
        }
        if (n == 0)
            return;

        received += (size_t)n;
        request[received] = '\0';

        if (find_header_end(request, received, &header_end))
            break;
    }

    if (header_end == 0) {
        send_error(fd, "400 Bad Request", "Invalid or incomplete request.");
        return;
    }

    header_status = parse_content_length(request, header_end, &content_length);
    if (header_status == -2) {
        send_error(fd, "413 Payload Too Large", "The request body is too large.");
        return;
    }
    if (header_status < 0) {
        send_error(fd, "400 Bad Request", "Invalid request headers.");
        return;
    }

    if (content_length > MAX_REQUEST - header_end) {
        send_error(fd, "413 Payload Too Large", "The request body is too large.");
        return;
    }

    while (received < header_end + content_length) {
        ssize_t n = recv(fd, request + received,
                         header_end + content_length - received, 0);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            return;
        }
        if (n == 0) {
            send_error(fd, "400 Bad Request", "Incomplete request body.");
            return;
        }
        received += (size_t)n;
        request[received] = '\0';
    }

    if (sscanf(request, "%15s %1023s", method, path) != 2) {
        send_error(fd, "400 Bad Request", "Invalid request line.");
        return;
    }

    if (strcmp(path, "/") != 0 && strcmp(path, "/index.html") != 0) {
        send_error(fd, "404 Not Found", "The requested page was not found.");
        return;
    }

    if (strcmp(method, "GET") == 0) {
        render_page(fd, "");
    } else if (strcmp(method, "POST") == 0) {
        size_t body_start = header_end;
        extract_message(request + body_start, content_length,
                        message, sizeof(message));
        render_page(fd, message);
    } else {
        send_error(fd, "405 Method Not Allowed", "Use GET or POST.");
    }
}

int main(void)
{
    int server_fd;
    int reuse = 1;
    struct sockaddr_in address;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd,