#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdint.h>

#define BUFFER_SIZE 512
#define MAX_DOMAIN_LEN 256

struct dns_header {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
};

int create_udp_socket(void) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        return -1;
    }
    return sockfd;
}

int bind_socket(int sockfd, const char *ip, int port) {
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        perror("invalid address");
        return -1;
    }

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        return -1;
    }
    return 0;
}

ssize_t receive_query(int sockfd, struct sockaddr_in *client_addr, unsigned char *buffer, size_t buf_size) {
    socklen_t addr_len = sizeof(*client_addr);
    ssize_t len = recvfrom(sockfd, buffer, buf_size, 0, (struct sockaddr *)client_addr, &addr_len);
    if (len < 0) {
        perror("recvfrom failed");
    }
    return len;
}

int parse_dns_header(const unsigned char *buffer, struct dns_header *header) {
    if (buffer == NULL || header == NULL) return -1;

    header->id = (buffer[0] << 8) | buffer[1];
    header->flags = (buffer[2] << 8) | buffer[3];
    header->qdcount = (buffer[4] << 8) | buffer[5];
    header->ancount = (buffer[6] << 8) | buffer[7];
    header->nscount = (buffer[8] << 8) | buffer[9];
    header->arcount = (buffer[10] << 8) | buffer[11];
    return 0;
}

int parse_domain_name(const unsigned char *buffer, int offset, char *domain, size_t domain_size) {
    int pos = offset;
    int domain_pos = 0;
    int jumped = 0;
    int original_pos = 0;

    while (buffer[pos] != 0) {
        if ((buffer[pos] & 0xC0) == 0xC0) {
            if (!jumped) {
                original_pos = pos + 2;
                jumped = 1;
            }
            pos = ((buffer[pos] & 0x3F) << 8) | buffer[pos + 1];
            continue;
        }

        int len = buffer[pos];
        pos++;

        if (domain_pos + len + 1 >= domain_size) return -1;

        for (int i = 0; i < len; i++) {
            domain[domain_pos++] = buffer[pos++];
        }
        domain[domain_pos++] = '.';
    }

    if (domain_pos > 0) {
        domain[domain_pos - 1] = '\0';
    } else {
        domain[0] = '\0';
    }

    if (!jumped) {
        return pos + 1;
    }
    return original_pos;
}

int parse_dns_question(const unsigned char *buffer, int offset, char *domain, size_t domain_size, uint16_t *qtype, uint16_t *qclass) {
    int new_offset = parse_domain_name(buffer, offset, domain, domain_size);
    if (new_offset < 0) return -1;

    *qtype = (buffer[new_offset] << 8) | buffer[new_offset + 1];
    *qclass = (buffer[new_offset + 2] << 8) | buffer[new_offset + 3];

    return new_offset + 4;
}

int build_dns_response(const unsigned char *query, size_t query_len, unsigned char *response, size_t *response_len, const char *resolve_ip) {
    if (query_len < 12) return -1;

    memcpy(response, query, query_len);

    response[2] = 0x81;
    response[3] = 0x80;

    response[6] = 0x00;
    response[7] = 0x01;

    response[8] = 0x00;
    response[9] = 0x00;

    response[10] = 0x00;
    response[11] = 0x00;

    size_t pos = query_len;

    response[pos++] = 0xC0;
    response[pos++] = 0x0C;

    response[pos++] = 0x00;
    response[pos++] = 0x01;

    response[pos++] = 0x00;
    response[pos++] = 0x01;

    response[pos++] = 0x00;
    response[pos++] = 0x00;
    response[pos++] = 0x00;
    response[pos++] = 0x3C;

    response[pos++] = 0x00;
    response[pos++] = 0x04;

    struct in_addr addr;
    if (inet_pton(AF_INET, resolve_ip, &addr) <= 0) {
        return -1;
    }

    memcpy(&response[pos], &addr, 4);
    pos += 4;

    *response_len = pos;
    return 0;
}