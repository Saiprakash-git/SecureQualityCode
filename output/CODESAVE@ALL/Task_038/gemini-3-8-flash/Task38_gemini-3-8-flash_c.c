#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080
#define BUFFER_SIZE 8192

static void url_decode(char *dst, size_t dst_size, const char *src) {
    size_t d_idx = 0;
    while (*src && d_idx + 1 < dst_size) {
        if (*src == '%' && isxdigit((unsigned char)src[1]) && isxdigit((unsigned char)src[2])) {
            char hex[3] = { src[1], src[2], '\0' };
            dst[d_idx++] = (char)strtol(hex, NULL, 16);
            src += 3;
        } else if (*src == '+') {
            dst[d_idx++] = ' ';
            src++;
        } else {
            dst[d_idx++] = *src++;
        }
    }
    dst[d_idx] = '\0';
}

static void html_escape(char *dst, size_t dst_size, const char *src) {
    size_t d_idx = 0;
    while (*src && d_idx + 7 < dst_size) {
        switch (*src) {
            case '&':
                memcpy(&dst[d_idx], "&amp;", 5);
                d_idx += 5;
                break;
            case '<':
                memcpy(&dst[d_idx], "&lt;", 4);
                d_idx += 4;
                break;
            case '>':
                memcpy(&dst[d_idx], "&gt;", 4);
                d_idx += 4;
                break;
            case '"':
                memcpy(&dst[d_idx], "&quot;", 6);
                d_idx += 6;
                break;
            case '\'':
                memcpy(&dst[d_idx], "&#39;", 5);
                d_idx += 5;
                break;
            default:
                dst[d_idx++] = *src;
                break;
        }
        src++;
    }
    dst[d_idx] = '\0';
}

static int extract_field_value(const char *body, const char *key, char *out_val, size_t out_size) {
    size_t key_len = strlen(key);
    const char *curr = body;

    while (curr && *curr) {
        const char *match = strstr(curr, key);
        if (!match) {
            return 0;
        }
        if ((match == body || *(match - 1) == '&') && match[key_len] == '=') {
            const char *val_start = match + key_len + 1;
            const char *val_end = strchr(val_start, '&');
            size_t val_len = val_end ? (size_t)(val_end - val_start) : strlen(val_start);
            char encoded[BUFFER_SIZE];

            if (val_len >= sizeof(encoded)) {
                val_len = sizeof(encoded) - 1;
            }
            memcpy(encoded, val_start, val_len);
            encoded[val_len] = '\0';

            url_decode(out_val, out_size, encoded);
            return 1;
        }
        curr = match + key_len;
    }
    return 0;
}

static void build_response(const char *submitted_text, int has_input, char *response_buf, size_t buf_size) {
    char escaped_text[BUFFER_SIZE] = {0};
    char content[BUFFER_SIZE] = {0};
    char display_section[BUFFER_SIZE] = {0};

    if (has_input) {
        html_escape(escaped_text, sizeof(escaped_text), submitted_text);
        snprintf(display_section, sizeof(display_section),
                 "<div style=\"margin-top:20px;padding:15px;background:#e7f3fe;border-left:5px solid #2196F3;\">"
                 "<strong>You submitted:</strong><p>%s</p>"
                 "</div>",
                 escaped_text);
    }

    snprintf(content, sizeof(content),
             "<!DOCTYPE html>"
             "<html><head><meta charset=\"UTF-8\"><title>Web Input App</title>"
             "<style>"
             "body { font-family: Arial, sans-serif; margin: 40px; background: #f9f9f9; }"
             ".container { max-width: 500px; margin: auto; background: white; padding: 25px; border-radius: 8px; box-shadow: 0 0 10px rgba(0,0,0,0.1); }"
             "input[type=text] { width: 100%%; padding: 10px; margin: 10px 0; box-sizing: border-box; border: 1px solid #ccc; border-radius: 4px; }"
             "input[type=submit] { background-color: #4CAF50; color: white; padding: 10px 15px; border: none; border-radius: 4px; cursor: pointer; }"
             "input[type=submit]:hover { background-color: #45a049; }"
             "</style></head>"
             "<body><div class=\"container\">"
             "<h2>Echo Form</h2>"
             "<form method=\"POST\" action=\"/\">"
             "<label for=\"user_input\">Enter your message:</label><br>"
             "<input type=\"text\" id=\"user_input\" name=\"user_input\" required>"
             "<input type=\"submit\" value=\"Submit\">"
             "</form>%s"
             "</div></body></html>",
             display_section);

    snprintf(response_buf, buf_size,
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html; charset=UTF-8\r\n"
             "Content-Length: %zu\r\n"
             "Connection: close\r\n\r\n"
             "%s",
             strlen(content), content);
}

static void handle_client(int client_fd) {
    char request[BUFFER_SIZE] = {0};
    char response[BUFFER_SIZE * 2] = {0};
    char user_input[BUFFER_SIZE] = {0};
    ssize_t bytes_read = read(client_fd, request, sizeof(request) - 1);

    if (bytes_read > 0) {
        request[bytes_read] = '\0';
        int has_input = 0;

        if (strncmp(request, "POST", 4) == 0) {
            char *body = strstr(request, "\r\n\r\n");
            if (body) {
                body += 4;
                has_input = extract_field_value(body, "user_input", user_input, sizeof(user_input));
            }
        }

        build_response(user_input, has_input, response, sizeof(response));
        write(client_fd, response, strlen(response));
    }

    close(client_fd);
}

int main(void) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    printf("Server running on http://localhost:%d\n", PORT);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd >= 0) {
            handle_client(client_fd);
        }
    }

    close(server_fd);
    return 0;
}