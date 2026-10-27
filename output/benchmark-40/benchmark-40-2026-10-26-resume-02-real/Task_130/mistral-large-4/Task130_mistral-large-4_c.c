#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdint.h>

#define DEFAULT_PORT 8080
#define BUFFER_SIZE 4096

static const char *cors_headers =
    "Access-Control-Allow-Origin: *\r\n"
    "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
    "Access-Control-Allow-Headers: Content-Type, Authorization, X-Requested-With, Accept, Origin\r\n"
    "Access-Control-Allow-Credentials: true\r\n"
    "Access-Control-Max-Age: 86400\r\n";

int send_response(int client_fd, const char *status, const char *body) {
    char response[BUFFER_SIZE];
    int body_len = (int)strlen(body);
    int response_len;

    response_len = snprintf(response, sizeof(response),
        "HTTP/1.1 %s\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %d\r\n"
        "%s"
        "\r\n"
        "%s",
        status, body_len, cors_headers, body);

    if (response_len < 0 || response_len >= (int)sizeof(response)) {
        return -1;
    }

    return (int)write(client_fd, response, (size_t)response_len);
}

int handle_client(int client_fd) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);

    if (bytes_read <= 0) {
        return -1;
    }

    buffer[bytes_read] = '\0';

    if (strncmp(buffer, "OPTIONS", 7) == 0) {
        return send_response(client_fd, "204 No Content", "");
    } else if (strncmp(buffer, "GET", 3) == 0) {
        return send_response(client_fd, "200 OK", "{\"message\":\"Hello from CORS-enabled API\",\"status\":\"success\",\"data\":{\"version\":\"1.0\"}}");
    } else if (strncmp(buffer, "POST", 4) == 0) {
        return send_response(client_fd, "201 Created", "{\"message\":\"Resource created\",\"status\":\"success\"}");
    } else if (strncmp(buffer, "PUT", 3) == 0) {
        return send_response(client_fd, "200 OK", "{\"message\":\"Resource updated\",\"status\":\"success\"}");
    } else if (strncmp(buffer, "DELETE", 6) == 0) {
        return send_response(client_fd, "200 OK", "{\"message\":\"Resource deleted\",\"status\":\"success\"}");
    } else {
        return send_response(client_fd, "404 Not Found", "{\"error\":\"Endpoint not found\",\"status\":\"error\"}");
    }
}

int main(int argc, char *argv[]) {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    int port = DEFAULT_PORT;

    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "Invalid port number\n");
            return EXIT_FAILURE;
        }
    }

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        return EXIT_FAILURE;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        close(server_fd);
        return EXIT_FAILURE;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons((uint16_t)port);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        return EXIT_FAILURE;
    }

    if (listen(server_fd, 10) < 0) {
        perror("Listen failed");
        close(server_fd);
        return EXIT_FAILURE;
    }

    printf("CORS-enabled API server listening on port %d\n", port);
    printf("Allowed origins: *\n");
    printf("Allowed methods: GET, POST, PUT, DELETE, OPTIONS\n");

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("Accept failed");
            continue;
        }

        handle_client(client_fd);
        close(client_fd);
    }

    close(server_fd);
    return EXIT_SUCCESS;
}