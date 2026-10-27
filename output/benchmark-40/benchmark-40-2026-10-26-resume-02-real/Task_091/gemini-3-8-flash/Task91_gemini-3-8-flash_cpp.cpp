#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdint>
#include <unordered_map>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>

#pragma pack(push, 1)
struct DnsHeader {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
};
#pragma pack(pop)

struct DnsQuestion {
    std::string qname;
    uint16_t qtype;
    uint16_t qclass;
    size_t length_in_packet;
};

// Step 1: Initialize and bind the UDP socket to the specified IP and port
int create_and_bind_socket(const std::string& ip_address, uint16_t port) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "Failed to create socket.\n";
        return -1;
    }

    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip_address.c_str(), &server_addr.sin_addr) <= 0) {
        std::cerr << "Invalid IP address: " << ip_address << "\n";
        close(sockfd);
        return -1;
    }

    if (bind(sockfd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
        std::cerr << "Failed to bind socket.\n";
        close(sockfd);
        return -1;
    }

    return sockfd;
}

// Step 2: Parse incoming raw UDP payload into DNS Query components
bool parse_dns_question(const uint8_t* buffer, size_t size, size_t offset, DnsQuestion& out_question) {
    std::string domain;
    size_t cursor = offset;

    while (cursor < size) {
        uint8_t label_len = buffer[cursor++];
        if (label_len == 0) {
            break;
        }
        if (cursor + label_len > size) {
            return false;
        }
        if (!domain.empty()) {
            domain += ".";
        }
        domain.append(reinterpret_cast<const char*>(&buffer[cursor]), label_len);
        cursor += label_len;
    }

    if (cursor + 4 > size) {
        return false;
    }

    out_question.qname = domain;
    out_question.qtype = ntohs(*reinterpret_cast<const uint16_t*>(&buffer[cursor]));
    cursor += 2;
    out_question.qclass = ntohs(*reinterpret_cast<const uint16_t*>(&buffer[cursor]));
    cursor += 2;
    out_question.length_in_packet = cursor - offset;

    return true;
}

// Step 3: Resolve DNS record via database or lookup table
bool resolve_record(const std::string& qname, uint16_t qtype, uint32_t& resolved_ip) {
    static const std::unordered_map<std::string, std::string> dns_zone = {
        {"example.com", "93.184.216.34"},
        {"localhost", "127.0.0.1"},
        {"dns.local", "192.168.1.1"}
    };

    if (qtype != 1) { // Type 1 corresponds to DNS A-Record
        return false;
    }

    auto it = dns_zone.find(qname);
    if (it != dns_zone.end()) {
        in_addr addr;
        if (inet_pton(AF_INET, it->second.c_str(), &addr) == 1) {
            resolved_ip = addr.s_addr;
            return true;
        }
    }
    return false;
}

// Step 4: Build DNS Response packet with Header, Question, and Answer sections
std::vector<uint8_t> build_dns_response(const uint8_t* query_buf, size_t query_len, const DnsQuestion& question, bool found, uint32_t resolved_ip) {
    std::vector<uint8_t> response;
    response.reserve(512);

    const DnsHeader* query_header = reinterpret_cast<const DnsHeader*>(query_buf);

    DnsHeader resp_header{};
    resp_header.id = query_header->id;
    
    // QR=1 (Response), AA=1 (Authoritative), RD preserved, RA=1, RCODE = 0 (NoError) or 3 (NXDomain)
    uint16_t flags = 0x8000;
    flags |= (ntohs(query_header->flags) & 0x0100); // Preserve Recursion Desired
    flags |= 0x0400; // Authoritative Answer
    flags |= 0x0080; // Recursion Available
    if (!found) {
        flags |= 0x0003; // NXDOMAIN
    }
    resp_header.flags = htons(flags);
    resp_header.qdcount = htons(1);
    resp_header.ancount = htons(found ? 1 : 0);
    resp_header.nscount = 0;
    resp_header.arcount = 0;

    const uint8_t* header_bytes = reinterpret_cast<const uint8_t*>(&resp_header);
    response.insert(response.end(), header_bytes, header_bytes + sizeof(DnsHeader));

    // Append Question Section exactly as received
    size_t question_offset = sizeof(DnsHeader);
    response.insert(response.end(), query_buf + question_offset, query_buf + question_offset + question.length_in_packet);

    if (found) {
        // Step 4b: Construct Answer Resource Record
        // Name pointer pointing to question offset (0xC000 | 12)
        uint16_t name_ptr = htons(0xC00C);
        const uint8_t* ptr_bytes = reinterpret_cast<const uint8_t*>(&name_ptr);
        response.insert(response.end(), ptr_bytes, ptr_bytes + 2);

        uint16_t type = htons(1); // Type A
        const uint8_t* type_bytes = reinterpret_cast<const uint8_t*>(&type);
        response.insert(response.end(), type_bytes, type_bytes + 2);

        uint16_t qclass = htons(1); // Class IN
        const uint8_t* class_bytes = reinterpret_cast<const uint8_t*>(&qclass);
        response.insert(response.end(), class_bytes, class_bytes + 2);

        uint32_t ttl = htonl(300); // 5 minutes TTL
        const uint8_t* ttl_bytes = reinterpret_cast<const uint8_t*>(&ttl);
        response.insert(response.end(), ttl_bytes, ttl_bytes + 4);

        uint16_t rdlength = htons(4); // IPv4 size
        const uint8_t* rdlen_bytes = reinterpret_cast<const uint8_t*>(&rdlength);
        response.insert(response.end(), rdlen_bytes, rdlen_bytes + 2);

        const uint8_t* ip_bytes = reinterpret_cast<const uint8_t*>(&resolved_ip);
        response.insert(response.end(), ip_bytes, ip_bytes + 4);
    }

    return response;
}

// Step 5: Send constructed packet back to client over UDP
ssize_t send_dns_response(int sockfd, const std::vector<uint8_t>& response, const sockaddr_in& client_addr) {
    return sendto(sockfd, response.data(), response.size(), 0, reinterpret_cast<const sockaddr*>(&client_addr), sizeof(client_addr));
}

// Main event loop dispatching the steps
void run_dns_server(const std::string& listen_ip, uint16_t listen_port) {
    int sockfd = create_and_bind_socket(listen_ip, listen_port);
    if (sockfd < 0) {
        return;
    }

    std::cout << "DNS server listening on " << listen_ip << ":" << listen_port << "\n";

    uint8_t buffer[512];
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);

    while (true) {
        ssize_t bytes_received = recvfrom(sockfd, buffer, sizeof(buffer), 0, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
        if (bytes_received < static_cast<ssize_t>(sizeof(DnsHeader))) {
            continue;
        }

        DnsQuestion question;
        if (!parse_dns_question(buffer, bytes_received, sizeof(DnsHeader), question)) {
            continue;
        }

        std::cout << "Received query for domain: " << question.qname << " [Type: " << question.qtype << "]\n";

        uint32_t resolved_ip = 0;
        bool found = resolve_record(question.qname, question.qtype, resolved_ip);

        std::vector<uint8_t> response = build_dns_response(buffer, bytes_received, question, found, resolved_ip);
        send_dns_response(sockfd, response, client_addr);
    }

    close(sockfd);
}

int main(int argc, char* argv[]) {
    std::string ip = "127.0.0.1";
    uint16_t port = 5353;

    if (argc >= 2) {
        ip = argv[1];
    }
    if (argc >= 3) {
        port = static_cast<uint16_t>(std::stoi(argv[2]));
    }

    run_dns_server(ip, port);
    return 0;
}