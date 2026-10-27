#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdatomic.h>

static atomic_ullong session_counter = 0;

static void fill_random_bytes(unsigned char *out, size_t len) {
    if (!out || len == 0) {
        return;
    }

    int fd = open("/dev/urandom", O_RDONLY);
    if (fd >= 0) {
        size_t total = 0;
        while (total < len) {
            ssize_t r = read(fd, out + total, len - total);
            if (r > 0) {
                total += (size_t)r;
            } else if (r < 0 && errno == EINTR) {
                continue;
            } else {
                break;
            }
        }
        close(fd);

        if (total == len) {
            return;
        }
    }

    unsigned long long state = (unsigned long long)time(NULL) ^
                               ((unsigned long long)getpid() << 16) ^
                               ((unsigned long long)clock() << 8) ^
                               (unsigned long long)(uintptr_t)out;

    if (state == 0) {
        state = 0x9E3779B97F4A7C15ULL;
    }

    for (size_t i = 0; i < len; ++i) {
        state ^= state << 13;
        state ^= state >> 7;
        state ^= state << 17;
        out[i] = (unsigned char)(state & 0xFF);
    }
}

static void hex_encode(const unsigned char *input, size_t input_len, char *output, size_t output_size) {
    static const char digits[] = "0123456789abcdef";

    if (!input || !output || output_size < input_len * 2 + 1) {
        return;
    }

    for (size_t i = 0; i < input_len; ++i) {
        output[i * 2] = digits[(input[i] >> 4) & 0x0F];
        output[i * 2 + 1] = digits[input[i] & 0x0F];
    }

    output[input_len * 2] = '\0';
}

char *generate_session_id(char *buffer, size_t buffer_size) {
    if (!buffer || buffer_size < 65) {
        return NULL;
    }

    unsigned char random_part[16];
    unsigned char unique_part[16];
    unsigned char payload[32];

    fill_random_bytes(random_part, sizeof(random_part));

    unsigned long long counter = atomic_fetch_add(&session_counter, 1ULL);
    uint64_t pid = (uint64_t)getpid();
    uint64_t ts