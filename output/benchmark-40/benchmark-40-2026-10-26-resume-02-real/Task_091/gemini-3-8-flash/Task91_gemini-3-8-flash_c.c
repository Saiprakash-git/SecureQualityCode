#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFFER_SIZE 512
#define DEFAULT_IP "127.0.0.1"
#define DEFAULT_PORT 5353

#define DNS_TYPE_A 1
#define DNS_CLASS_IN 1

/*
 * High-Level DNS Server Processing Steps:
 * 1. Initialize and bind a UDP socket to a designated IP and port.
 * 2. Listen in an event loop for incoming datagrams (DNS queries).
 * 3. Parse the DNS header and question section (extract transaction ID, QNAME, QTYPE).
 * 4. Resolve the requested record (match domain to an IP address).
 * 5. Construct a standardized DNS response packet with matching ID, flags, and answer RRs.
 * 6. Transmit the response back to the client's network address via UDP.
 */

#pragma pack(push, 1)
struct dns_header {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
};
#pragma pack(pop)

struct dns_question {
    char name[256];
    uint16_t type;
    uint16_t qclass;
    size_t wire_length;
};

int setup_udp_socket(const char *ip_str, uint16_t port) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return -1;
    }

    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(sockfd);
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip_str, &server_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        close(sockfd);
        return -1;
    }

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        return -1;
    }

    return sockfd;
}

int parse_dns_question(const uint8_t *buffer, size_t buf_len, size_t offset, struct dns_question *q) {
    size_t idx = offset;
    size_t name_idx = 0;

    memset(q->name, 0, sizeof(q->name));

    while (idx < buf_len && buffer[idx] != 0) {
        uint8_t label_len = buffer[idx++];
        if (idx + label_len > buf_len || name_idx + label_len + 1 >= sizeof(q->name)) {
            return -1;
        }

        for (uint8_t i = 0; i < label_len; i++) {
            q->name[name_idx++] = (char)buffer[idx++];
        }
        q->name[name_idx++] = '.';
    }

    if (idx >= buf_len || buffer[idx] != 0) {
        return -1;
    }
    idx++;

    if (name_idx > 0) {
        q->name[name_idx - 1] = '\0';
    } else {
        q->name[0] = '\0';
    }

    if (idx + 4 > buf_len) {
        return -1;
    }

    q->type = ntohs(*(const uint16_t *)(buffer + idx));
    idx += 2;
    q->qclass = ntohs(*(const uint16_t *)(buffer + idx));
    idx += 2;

    q->wire_length = idx - offset;
    return 0;
}

int resolve_record(const char *domain, uint16_t type, uint32_t *out_ipv4) {
    if (type != DNS_TYPE_A) {
        return -1;
    }

    if (strcmp(domain, "example.com") == 0) {
        inet_pton(AF_INET, "93.184.216.34", out_ipv4);
        return 0;
    } else if (strcmp(domain, "localhost") == 0) {
        inet_pton(AF_INET, "127.0.0.1", out_ipv4);
        return 0;
    }

    inet_pton(AF_INET, "192.168.1.1", out_ipv4);
    return 0;
}

ssize_t build_dns_response(const uint8_t *query_buf, size_t query_len,
                           const struct dns_question *q, uint32_t resolved_ip,
                           int found, uint8_t *resp_buf, size_t max_resp_len) {
    if (max_resp_len < sizeof(struct dns_header) + q->wire_length + 16) {
        return -1;
    }

    const struct dns_header *req_hdr = (const struct dns_header *)query_buf;
    struct dns_header *res_hdr = (struct dns_header *)resp_buf;

    res_hdr->id = req_hdr->id;

    uint16_t flags = 0x8000;
    flags |= (ntohs(req_hdr->flags) & 0x0100);
    flags |= 0x0080;

    if (!found) {
        flags |= 0x0003;
        res_hdr->flags = htons(flags);
        res_hdr->qdcount = htons(1);
        res_hdr->ancount = htons(0);
        res_hdr->nscount = 0;
        res_hdr->arcount = 0;

        memcpy(resp_buf + sizeof(struct dns_header),
               query_buf + sizeof(struct dns_header),
               q->wire_length);

        return sizeof(struct dns_header) + q->wire_length;
    }

    res_hdr->flags = htons(flags);
    res_hdr->qdcount = htons(1);
    res_hdr->ancount = htons(1);
    res_hdr->nscount = 0;
    res_hdr->arcount = 0;

    size_t offset = sizeof(struct dns_header);
    memcpy(resp_buf + offset, query_buf + sizeof(struct dns_header), q->wire_length);
    offset += q->wire_length;

    resp_buf[offset++] = 0xC0;
    resp_buf[offset++] = sizeof(struct dns_header);

    uint16_t ans_type = htons(DNS_TYPE_A);
    memcpy(resp_buf + offset, &ans_type, 2);
    offset += 2;

    uint16_t ans_class = htons(DNS_CLASS_IN);
    memcpy(resp_buf + offset, &ans_class, 2);
    offset += 2;

    uint32_t ttl = htonl(300);
    memcpy(resp_buf + offset, &ttl, 4);
    offset += 4;

    uint16_t data_len = htons(4);
    memcpy(