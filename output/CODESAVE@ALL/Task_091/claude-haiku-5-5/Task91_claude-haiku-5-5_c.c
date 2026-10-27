#define _DEFAULT_SOURCE
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

/*
 * Minimal authoritative/forwarding UDP DNS server.
 *
 * Steps:
 *  1. Parse command line: listen IP, listen port, optional upstream resolver IP.
 *  2. Create a UDP socket (AF_INET, SOCK_DGRAM) and bind it to listen IP:port.
 *  3. Loop forever:
 *     a. recvfrom() a datagram and remember the client address.
 *     b. Parse the DNS header (12 bytes): check QR=0, opcode=0, QDCOUNT=1.
 *     c. Parse the question name (length-prefixed labels, lowercased), QTYPE, QCLASS.
 *     d. Resolve:
 *        - If the name is in the local zone table and type A/IN: build an answer.
 *        - Else if an upstream resolver is configured: forward the raw query over UDP,
 *          wait up to 2 seconds, and relay the upstream reply.
 *        - Else: return NXDOMAIN.
 *     e. Malformed input gets FORMERR; upstream failure gets SERVFAIL.
 *  4. sendto() the response back to the client address (same IP and port it came from).
 */

#define DNS_HDR_LEN   12
#define DNS_BUF_SIZE  4096
#define DNS_TYPE_A    1
#define DNS_CLASS_IN  1
#define DNS_RCODE_FORMERR  1
#define DNS_RCODE_SERVFAIL 2
#define DNS_RCODE_NXDOMAIN 3

struct local_record {
    const char *name;
    uint8_t ip[4];
    uint32_t ttl;
};

static const struct local_record zone[] = {
    {"example.local",     {192, 0, 2, 10}, 300},
    {"www.example.local", {192, 0, 2, 11}, 300},
    {"api.example.local", {192, 0, 2, 12}, 60},
};

/* Parse a (possibly root) domain name at buf[off]. Compression pointers are rejected in queries. */
static int parse_name(const uint8_t *buf, size_t len, size_t off,
                      char *out, size_t outsz, size_t *next)
{
    size_t pos = off, outlen = 0;

    for (;;) {
        if (pos >= len) return -1;
        uint8_t l = buf[pos];
        if (l & 0xC0) return -1;          /* pointer or reserved bits: not allowed here