#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

static int fill_random_bytes(unsigned char *buffer, size_t length)
{
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0)
        return -1;

    size_t offset = 0;
    while (offset < length) {
        ssize_t count = read(fd, buffer + offset, length - offset);
        if (count < 0 && errno == EINTR)
            continue;
        if (count <= 0) {
            int saved_errno = count == 0 ? EIO : errno;
            close(fd);
            errno = saved_errno;
            return -1;
        }
        offset += (size_t)count;
    }

    if (close(fd) < 0)
        return -1;

    return 0;
}

int generate_session_id(char *output, size_t output_size)
{
    static const char hex[] = "0123456789abcdef";
    static const char positions[] = {
        0, 1, 2, 3, 4, 5, 6, 7,
        9, 10, 11, 12,
        14, 15, 16, 17,
        19, 20, 21, 22, 23, 24,
        26, 27, 28, 29, 30, 31,
        32, 33, 34, 35
    };
    unsigned char random_bytes[16];

    if (output == NULL || output_size < 37) {
        errno = EINVAL;
        return -1;
    }

    if (fill_random_bytes(random_bytes, sizeof random_bytes) < 0)
        return -1;

    random_bytes[6] = (unsigned char)((random_bytes[6] & 0x0f) | 0x40);
    random_bytes[8] = (unsigned char)((random_bytes[8] & 0x3f) | 0x80);

    size_t byte_index = 0;
    for (size_t i = 0; i < 36; ++i) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            output[i] = '-';
        } else {
            unsigned char value = random_bytes[byte_index / 2];
            output[i] = hex[(byte_index % 2 == 0) ? value >> 4 : value & 0x0f];
            ++byte_index;
        }
    }
    output[36] = '\0';

    return 0;
}

int main(void)
{
    char session_id[37];

    if (generate_session_id(session_id, sizeof session_id) < 0) {
        perror("generate_session_id");
        return 1;
    }

    puts(session_id);
    return 0;
}