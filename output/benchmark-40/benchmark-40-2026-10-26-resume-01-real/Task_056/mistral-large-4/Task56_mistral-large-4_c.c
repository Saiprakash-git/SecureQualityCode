#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#include <wincrypt.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

#define DEFAULT_TOKEN_BYTES 32
#define MAX_TOKEN_BYTES 256

int get_secure_random(unsigned char *buf, size_t len) {
#ifdef _WIN32
    HCRYPTPROV prov;
    if (!CryptAcquireContext(&prov, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        return -1;
    }
    BOOL result = CryptGenRandom(prov, (DWORD)len, buf);
    CryptReleaseContext(prov, 0);
    return result ? 0 : -1;
#else
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) return -1;
    size_t off = 0;
    while (off < len) {
        ssize_t n = read(fd, buf + off, len - off);
        if (n <= 0) {
            close(fd);
            return -1;
        }
        off += (size_t)n;
    }
    close(fd);
    return 0;
#endif
}

char* generate_auth_token(const char *user_id, size_t num_bytes) {
    if (num_bytes == 0) num_bytes = DEFAULT_TOKEN_BYTES;
    if (num_bytes > MAX_TOKEN_BYTES) num_bytes = MAX_TOKEN_BYTES;

    unsigned char *random_data = malloc(num_bytes);
    if (!random_data) return NULL;

    if (get_secure_random(random_data, num_bytes) != 0) {
        free(random_data);
        return NULL;
    }

    size_t hex_len = num_bytes * 2;
    char *token = malloc(hex_len + 1);
    if (!token) {
        free(random_data);
        return NULL;
    }

    const char *hex = "0123456789abcdef";
    for (size_t i = 0; i < num_bytes; i++) {
        token[i * 2] = hex[(random_data[i] >> 4) & 0x0F];
        token[i * 2 + 1] = hex[random_data[i] & 0x0F];
    }
    token[hex_len] = '\0';

    free(random_data);
    return token;
}

int main(int argc, char **argv) {
    const char *user_id = (argc > 1) ? argv[1] : "default_user";
    size_t token_bytes = (argc > 2) ? (size_t)atoi(argv[2]) : DEFAULT_TOKEN_BYTES;

    char *token = generate_auth_token(user_id, token_bytes);
    if (token) {
        printf("%s\n", token);
        free(token);
        return 0;
    } else {
        fprintf(stderr, "Token generation failed\n");
        return 1;
    }
}