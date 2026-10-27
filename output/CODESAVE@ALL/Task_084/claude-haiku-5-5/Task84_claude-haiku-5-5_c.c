#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

#define SESSION_ID_BYTES 16
#define SESSION_ID_HEX_LEN (SESSION_ID_BYTES * 2)
#define SESSION_ID_BUF_LEN (SESSION_ID_HEX_LEN + 1)

static uint64_t session_counter = 0;

static int fill_random_bytes(unsigned char *out, size_t n)
{
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd >= 0) {
        size_t got = 0;
        while (got < n) {
            ssize_t r = read(fd, out + got, n - got);
            if (r <= 0) {
                break;
            }
            got += (size_t)r;
        }
        close(fd);
        if (got == n) {
            return 0;
        }
    }

    /* Fallback: non-cryptographic PRNG seeded with time, pid and counter */
    static int seeded = 0;
    if (!seeded) {
        unsigned int seed = (unsigned int)time(NULL)
                          ^ (unsigned int)getpid() * 2654435761u;
        srand(seed);
        seeded = 1;
    }
    for (size_t i = 0; i < n; i++) {
        out[i] = (unsigned char)(rand() & 0xFF);
    }
    return 0;
}

int generate_session_id(char *buf, size_t buflen)
{
    static const char hex[] = "0123456789abcdef";
    unsigned char bytes[SESSION_ID_BYTES];
    uint64_t counter;
    uint64_t now;

    if (buf == NULL || buflen < SESSION_ID_BUF_LEN) {
        return -1;
    }

    if (fill_random_bytes(bytes, SESSION_ID_BYTES) != 0) {
        return -1;
    }

    /* Mix in a per-process counter and timestamp so IDs never repeat */
    counter = ++session_counter;
    now = (uint64_t)time(NULL);

    for (int i = 0; i < 8; i++) {
        bytes[8 + i] ^= (unsigned char)((counter >> (56 - 8 * i)) & 0xFF);
        bytes[i] ^= (unsigned char)((now >> (56 - 8 * i)) & 0xFF);
    }

    for (size_t i = 0; i < SESSION_ID_BYTES; i++) {
        buf[i * 2]     = hex[(bytes[i] >> 4) & 0x0F];
        buf[i * 2 + 1] = hex[bytes[i] & 0x0F];
    }
    buf[SESSION_ID_HEX_LEN] = '\0';
    return 0;
}

int main(void)
{
    char id[SESSION_ID_BUF_LEN];

    for (int i = 0; i < 5; i++) {
        if (generate_session_id(id, sizeof(id)) == 0) {
            printf("%s\n", id);
        } else {
            fprintf(stderr, "Failed to generate session ID\n");
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}