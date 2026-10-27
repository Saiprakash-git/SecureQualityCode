```cpp
// UDP DNS server outline:
// 1) Create a UDP socket, bind it to the requested IPv4 address and port.
// 2) Receive a datagram and record the sender's address.
// 3) Parse the 12-byte DNS header (ID, flags, section counts).
// 4) Parse the question section (QNAME with label and compression-pointer handling, QTYPE, QCLASS).
// 5) Resolve the question against the local authoritative zone (A, AAAA, TXT).
// 6) Build the response: header, echoed question, answers using name compression.
// 7) Send the response back to the sender with sendto().

#include <algorithm>
#include <arpa/inet.h>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <optional>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

namespace dns {

constexpr uint16_t TYPE_A = 1;
constexpr uint16_t TYPE_TXT = 16;
constexpr uint16_t TYPE_AAAA = 28;
constexpr uint16_t CLASS_IN = 1;

constexpr uint8_t RCODE_NOERROR = 0;
constexpr uint8_t RCODE_FORMERR = 1;
constexpr uint8_t RCODE_NXDOMAIN = 3;
constexpr uint8_t RCODE_NOTIMP = 4;

struct Header {
    uint16_t id;
    uint16_t flags;
    uint16_t qd;
    uint16_t an;
    uint16_t ns;
    uint16_t ar;
};

struct Question {
    std::string name;
    uint16_t qtype;
    uint16_t qclass;
    size_t end;
};

struct Record {
    uint16_t type;
    uint32_t ttl;
    std::vector<uint8_t> rdata;
};

struct Zone {
    std::map<std::string, std::vector<Record>> rrsets;
};

struct Resolution {
    uint8_t rcode;
    std::vector<Record> answers;
};

// ---------- Byte helpers ----------

std::optional<uint16_t> read16(const std::vector<uint8_t>& buf, size_t off) {
    if (off + 2 > buf.size()) return std::nullopt;