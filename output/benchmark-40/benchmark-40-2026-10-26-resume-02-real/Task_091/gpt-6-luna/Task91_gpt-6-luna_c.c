#define _POSIX_C_SOURCE 200112L

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define DNS_HEADER_SIZE 12
#define DNS_MAX_RESPONSE 512
#define DNS_TTL 60

static uint16_t read_u16(const uint8_t *p)
{
    return (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
}

static void write_u16(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)(value >> 8);
    p[1] = (uint8_t)value;
}

static void write_u32(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)(value >> 24);
    p[1] = (uint8_t)(value >> 16);
    p[2] = (uint8_t)(value >> 8);
    p[3] = (uint8_t)value;
}

/*
 * Decode a DNS name, following compression pointers when present. The returned
 * consumed count is the number of bytes occupied at the original offset.
 */
static int decode_name(const uint8_t *packet, size_t packet_len, size_t offset,
                       char *name, size_t name_capacity, size_t *consumed)
{
    size_t pos = offset;
    size_t output_len = 0;
    size_t wire_consumed = 0;
    size_t steps = 0;
    int jumped = 0;
    int first_label = 1;

    if (offset >= packet_len || name_capacity == 0)
        return -1;

    for (;;) {
        uint8_t label_len;

        if (pos >= packet_len || ++steps > packet_len + 1)
            return -1;

        label_len = packet[pos];

        if ((label_len & 0xc0) == 0xc0) {
            size_t pointer;

            if (pos + 1 >= packet_len)
                return -1;

            pointer = ((size_t)(label_len & 0x3f) << 8) | packet[pos + 1];
            if (pointer >= packet_len)
                return -1;

            if (!jumped)
                wire_consumed += 2;

            jumped = 1;
            pos = pointer;
            continue;
        }

        if ((label_len & 0xc0) != 0)
            return -1;

        pos++;
        if (!jumped)
            wire_consumed++;

        if (label_len == 0)
            break;

        if (label_len > 63 || pos + label_len > packet_len)
            return -1;

        if (!first_label) {
            if (output_len + 1 >= name_capacity)
                return -1;
            name[output_len++] = '.';
        }

        if (output_len + label_len >= name_capacity)
            return -1;

        for (size_t i = 0; i < label_len; i++) {
            unsigned char c = packet[pos + i];

            /* This server uses the system hostname resolver. */
            if (c < 33 || c > 126 || c == '.')
                return -1;

            name[output_len++] = (char)c;
        }

        if (!jumped)
            wire_consumed += label_len;

        pos += label_len;
        first_label = 0;
    }

    if (first_label) {
        if (name_capacity < 2)
            return -1;
        name[output_len++] = '.';
    }

    name[output_len] = '\0';
    *cons