#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#if defined(_WIN32)
#include <windows.h>
#include <wincrypt.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

#define SESSION_ID_LEN 37

int get_random_bytes(unsigned char *buffer, size_t length) {
    if (buffer == NULL || length == 0) {
        return -1;
    }

#if defined(_WIN32)
    HCRYPTPROV provider;
    if (CryptAcquireContext(&provider, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_SILENT)) {
        if (CryptGenRandom(provider, (DWORD)length, buffer)) {
            CryptReleaseContext(provider, 0);
            return 0;
        }
        CryptReleaseContext(provider, 0);
    }
#else
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd >= 0) {
        size_t total_read = 0;
        while (total_read < length) {
            ssize_t result = read(fd, buffer + total_read, length - total_read);
            if (result <= 0) {
                break;
            }
            total_read += (size_t)result;
        }
        close(fd);
        if (total_read == length) {
            return 0;
        }
    }
#endif

    static int seeded = 0;
    if (!seeded) {
        srand((unsigned int)(time(NULL) ^ (uintptr_t)&seeded));
        seeded = 1;
    }

    for (size_t i = 0; i < length; ++i) {
        buffer[i] = (unsigned char)(rand() % 256);
    }

    return 0;
}

char *generate_session_id(char *destination, size_t max_len) {
    if (destination == NULL || max_len < SESSION_ID_LEN) {
        return NULL;
    }

    unsigned char raw_bytes[16];
    if (get_random_bytes(raw_bytes, sizeof(raw_bytes)) != 0) {
        return NULL;
    }

    raw_bytes[6] = (raw_bytes[6] & 0x0F) | 0x40;
    raw_bytes[8] = (raw_bytes[8] & 0x3F) | 0x80;

    snprintf(destination, max_len,
             "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
             raw_bytes[0], raw_bytes[1], raw_bytes[2], raw_bytes[3],
             raw_bytes[4], raw_bytes[5],
             raw_bytes[6], raw_bytes[7],
             raw_bytes[8], raw_bytes[9],
             raw_bytes[10], raw_bytes[11], raw_bytes[12], raw_bytes[13], raw_bytes[14], raw_bytes[15]);

    return destination;
}

int main(void) {
    char session_id[SESSION_ID_LEN];

    if (generate_session_id(session_id, sizeof(session_id)) != NULL) {
        printf("%s\n", session_id);
        return 0;
    }

    return 1;
}