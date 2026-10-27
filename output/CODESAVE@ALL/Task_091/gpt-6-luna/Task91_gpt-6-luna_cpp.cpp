#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>
#include <string>
#include <vector>

static uint16_t readU16(const unsigned char* data, size_t offset) {
    return static_cast<uint16_t>((data[offset] << 8) | data[offset + 1]);
}

static void writeU16(std::vector<unsigned char>& out, uint16_t value) {
    out.push_back(static_cast<unsigned char>((value >> 8) & 0xff));
    out.push_back(static_cast<unsigned char>(value & 0xff));
}

static void writeU32(std::vector<unsigned char>& out, uint32_t value) {
    out.push_back(static_cast<unsigned char>((value >> 24) & 0xff));
    out.push_back(static_cast<unsigned char>((value >> 16) & 0xff));
    out.push_back(static_cast<unsigned char>((value >> 8) & 0xff));
    out.push_back(static_cast<unsigned char>(value & 0xff));
}

// Decode a DNS name, including compression pointers. consumed counts only bytes
// occupied at the original offset, so the caller can locate QTYPE and QCLASS.
static bool decodeName(const unsigned char* packet, size_t length, size_t offset,
                       std::string& name, size_t& consumed) {
    name.clear();
    consumed = 0;
    size_t position = offset;
    bool jumped = false;
    unsigned int hops = 0;

    while (position < length && hops++ < 128) {
        const unsigned char label = packet[position];

        if ((label & 0xc0) == 0xc0) {
            if (position + 1 >= length) return false;
            const size_t target =
                static_cast<size_t>(((label & 0x3f) << 8) | packet[position + 1]);
            if (target >= length) return false;
            if (!jumped) consumed += 2;
            position = target;
            jumped = true;
            continue;
        }

        if ((label & 0xc0) != 0) return false;

        ++position;
        if (!jumped) ++consumed;

        if (label == 0) {
            if (name.empty()) name = ".";
            return true;
        }

        if (label > 63 || position + label > length) return false;
        if (!name.empty()) name.push_back('.');
        for (unsigned int i = 0; i < label; ++i) {
            const unsigned char ch = packet[position + i];
            if (ch == '\0') return false;
            name.push_back(static_cast<char>(ch));
        }

        position += label;
        if (!jumped) consumed += label;
    }

    return false;
}

struct Question {
    std::string name;
    uint16_t type = 0;
    uint16_t qclass = 0;
    size_t end = 0;
};

static bool parseQuestion(const unsigned char* packet, size_t length, Question& question) {
    if (length < 12 || readU16(packet, 4) != 1) return false;

    size_t nameLength = 0;
    if (!decodeName(packet, length, 12, question.name, nameLength)) return false;

    const size_t fields = 12 + nameLength;
    if (fields + 4 > length) return false;

    question.type = readU16(packet, fields);
    question.qclass = readU16(packet, fields + 2);
    question.end = fields + 4;
    return true;
}

// Construct a DNS response header and, when valid, copy the original question.
static std::vector<unsigned char> makeResponse(const unsigned char* query,
                                                size_t queryLength,
                                                const Question* question,
                                                uint16_t rcode) {
    std::vector<unsigned char> response(12, 0);
    if (queryLength >= 2) {
        response[0] = query[0];
        response[1] = query[1];
    }

    uint16_t queryFlags = queryLength >= 4 ? readU16(query, 2) : 0;
    uint16_t flags = static_cast<uint16_t>(
        0x8000 | (queryFlags & 0x7800) | (queryFlags & 0x0100) | 0x0080 | (rcode & 0x000f));
    response[2] = static_cast<unsigned char>(flags >> 8);
    response[3] = static_cast<unsigned char>(flags & 0xff);

    if (question && question->end <= queryLength) {
        response[5] = 1;
        response.insert(response.end(), query + 12, query + question->end);
    }
    return response;
}

static void setAnswerCount(std::vector<unsigned char>& response, uint16_t count) {
    response[6] = static_cast<unsigned char>(count >> 8);
    response[7] = static_cast<unsigned char>(count & 0xff);
}

static void setTruncated(std::vector<unsigned char>& response) {
    uint16_t flags = readU16(response.data(), 2);
    flags |= 0x0200;
    response[2] = static_cast<unsigned char>(flags >> 8);
    response[3] = static_cast<unsigned char>(flags & 0xff);
}

// Resolve A and AAAA questions with the system resolver and encode address records.
static std::vector<unsigned char> answerQuery(const unsigned char* query,
                                               size_t queryLength,
                                               const Question& question) {
    if (queryLength < 4) return makeResponse(query, queryLength, &question, 1);

    const uint16_t queryFlags = readU16(query, 2);
    if (queryFlags & 0x8000) return {};

    if (((queryFlags >> 11) & 0x0f) != 0)
        return makeResponse(query, queryLength, &question, 4);

    if (question.qclass != 1)
        return makeResponse(query, queryLength, &question, 4);

    if (question.type != 1 && question.type != 28)
        return makeResponse(query, queryLength, &question, 4);

    std::vector<unsigned char> response = makeResponse(query, queryLength, &question, 0);
    if (response.size() > 512) {
        setTruncated(response);
        return response;
    }

    addrinfo hints{};
    hints.ai_family = question.type == 1 ? AF_INET : AF_INET6;
    hints.ai_socktype = SOCK_DGRAM;

    addrinfo* results = nullptr;
    const int gaiResult = getaddrinfo(question.name.c_str(), nullptr, &hints, &results);
    if (gaiResult != 0) {
        const uint16_t rcode = gaiResult == EAI_NONAME ? 3 : 2;
        response = makeResponse(query, queryLength, &question, rcode);
        return response;
    }

    const size_t addressLength = question.type == 1 ? 4 : 16;
    std::set<std::string> seen;
    uint16_t answerCount = 0;
    bool truncated = false;

    for (addrinfo* current = results; current; current = current->ai_next) {
        const void* address = nullptr;
        if (question.type == 1 && current->ai_family == AF_INET) {
            address = &reinterpret_cast<sockaddr_in*>(current->ai_addr)->sin_addr;
        } else if (question.type == 28 && current->ai_family == AF_INET6) {
            address = &reinterpret_cast<sockaddr_in6*>(current->ai_addr)->sin6_addr;
        }
        if (!address) continue;

        std::string key(static_cast<const char*>(address), addressLength);
        if (!seen.insert(key).second) continue;

        std::vector<unsigned char> record;
        writeU16(record, 0xc00c);                 // Compressed owner name: QNAME at offset 12.
        writeU16(record, question.type);
        writeU16(record, 1);                      // IN class.
        writeU32(record, 60);                     // TTL.
        writeU16(record, static_cast<uint16_t>(addressLength));
        const unsigned char* bytes = static_cast<const unsigned char*>(address);
        record.insert(record.end(), bytes, bytes + addressLength);

        if (response.size() + record.size() > 512) {
            truncated = true;
            break;
        }

        response.insert(response.end(), record.begin(), record.end());
        ++answerCount;
    }

    freeaddrinfo(results);
    setAnswerCount(response, answerCount);
    if (truncated) setTruncated(response);
    return response;
}

static int createListener(const char* bindAddress, const char* port) {
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV;

    addrinfo* addresses = nullptr;
    const int result = getaddrinfo(bindAddress, port, &hints, &addresses);
    if (result != 0) {
        std::fprintf(stderr, "Invalid bind address or port: %s\n", gai_strerror(result));
        return -1;
    }

    int listener = -1;
    for (addrinfo* current = addresses; current; current = current->ai_next) {
        listener = socket(current->ai_family, current->ai_socktype, current->ai_protocol);
        if (listener < 0) continue;

        int reuse = 1;
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
        if (bind(listener, current->ai_addr, current->ai_addrlen) == 0) break;

        close(listener);
        listener = -1;
    }

    freeaddrinfo(addresses);
    return listener;
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::fprintf(stderr, "Usage: %s <bind-ip> <port>\n", argv[0]);
        return 1;
    }

    // Bind a UDP socket to the requested address and port.
    const int listener = createListener(argv[1], argv[2]);
    if (listener < 0) {
        std::perror("Could not create or bind UDP socket");
        return 1;
    }

    // Receive DNS packets, parse the question, resolve it, then reply to the sender.
    std::vector<unsigned char> packet(65535);
    for (;;) {
        sockaddr_storage client{};
        socklen_t clientLength = sizeof(client);
        const ssize_t received =
            recvfrom(listener, packet.data(), packet.size(), 0,
                     reinterpret_cast<sockaddr*>(&client), &clientLength);

        if (received <