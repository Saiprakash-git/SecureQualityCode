#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <vector>
#include <string>
#include <cstdint>

#pragma pack(push, 1)
struct DNSHeader {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
};
#pragma pack(pop)

std::string parseDomainName(const uint8_t* data, size_t& offset, size_t maxLen) {
    std::string result;
    while (offset < maxLen) {
        uint8_t len = data[offset++];
        if (len == 0) break;
        if ((len & 0xC0) == 0xC0) {
            if (offset >= maxLen) break;
            uint16_t pointer = ((len & 0x3F) << 8) | data[offset++];
            size_t savedOffset = offset;
            offset = pointer;
            result += parseDomainName(data, offset, maxLen);
            offset = savedOffset;
            return result;
        }
        if (offset + len > maxLen) break;
        if (!result.empty()) result += ".";
        result.append(reinterpret_cast<const char*>(data + offset), len);
        offset += len;
    }
    return result;
}

void buildDNSResponse(const uint8_t* query, size_t queryLen, uint8_t* response, size_t& responseLen, uint32_t resolvedIP) {
    DNSHeader* header = reinterpret_cast<DNSHeader*>(response);
    const DNSHeader* queryHeader = reinterpret_cast<const DNSHeader*>(query);

    header->id = queryHeader->id;
    header->flags = htons(0x8180);
    header->qdcount = queryHeader->qdcount;
    header->ancount = queryHeader->qdcount;
    header->nscount = 0;
    header->arcount = 0;

    size_t offset = sizeof(DNSHeader);
    size_t qOffset = sizeof(D