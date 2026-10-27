#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

enum {
    TOKEN_BYTES = 32,
    TOKEN_HEX_CHARS = TOKEN_BYTES * 2
};

static int generate_auth_token(char *output, size_t output_size)
{
    static const char hex[] = "0123456789abcdef";
    unsigned char random_bytes[TOKEN_BYTES];
    size_t offset = 0;
    int fd;

    if (output == NULL || output_size < TOKEN_HEX_CHARS + 1) {
        return -1;
    }

    fd = open("/dev/urandom", O_RDONLY);
    if (fd == -1) {
        return -1;
    }

    while (offset < sizeof random_bytes) {
        ssize_t count = read(fd, random_bytes + offset,
                             sizeof random_bytes - offset);
        if (count > 0) {
            offset += (size_t)count;
        } else if (count == -1 && errno == EINTR) {
            continue;
        } else {
            close(fd);
            return -1;
        }
    }

    if (close(fd) == -1) {
        return -1;
    }

    for (size_t i = 0; i < sizeof random_bytes; ++i) {
        output[i * 2] = hex[random_bytes[i] >> 4];
        output[i * 2 + 1] = hex[random_bytes[i] & 0x0f];
    }
    output[TOKEN_HEX_CHARS] = '\0';

    for (size_t i = 0; i < sizeof random_bytes; ++i) {
        ((volatile unsigned char *)random_bytes)[i] = 0;
    }

    return 0;
}

int main(void)
{
    char token[TOKEN_HEX_CHARS + 1];

    if (generate_auth_token(token, sizeof token) != 0) {
        fputs("Failed to generate authentication token.\n", stderr);
        return 1;
    }

    if (printf("%s\n", token) < 0) {
        return 1;
    }

    return 0;
}