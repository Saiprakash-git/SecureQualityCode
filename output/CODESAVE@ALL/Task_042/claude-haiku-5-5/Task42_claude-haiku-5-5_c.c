```c
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#define SHA256_DIGEST_LEN 32
#define SALT_LEN 16
#define HASH_LEN 32
#define PBKDF2_ITERATIONS 100000u
#define MAX_USERNAME_LEN 32
#define MIN_USERNAME_LEN 3
#define MIN_PASSWORD_LEN 8
#define MAX_PASSWORD_LEN 128
#define MAX_USERS 32
#define MAX_FAILED_ATTEMPTS 5

typedef enum {
    AUTH_OK = 0,
    AUTH_ERR_INVALID_ARGS,
    AUTH_ERR_INVALID_USERNAME,
    AUTH_ERR_WEAK_PASSWORD,
    AUTH_ERR_USER_EXISTS,
    AUTH_ERR_USER_NOT_FOUND,
    AUTH_ERR_BAD_CREDENTIALS,
    AUTH_ERR_LOCKED,
    AUTH_ERR_STORE_FULL,
    AUTH_ERR_RANDOM,
    AUTH_ERR_INTERNAL
} auth_result;

typedef struct {
    char name[MAX_USERNAME_LEN + 1];
    uint8_t salt[SALT_LEN];
    uint8_t hash[HASH_LEN];
    uint32_t iterations;
    unsigned int failures;
    int locked;
    int used;
} auth_user;

typedef struct {
    auth_user users[MAX_USERS];
    size_t count;
} auth_store;

/* ---------------- Secure memory helpers ---------------- */

static void secure_zero(void *p, size_t n) {
    volatile uint8_t *v = (volatile uint8_t *)p;
    while (n--) {
        *v++ = 0;
    }
}

static int ct_equal(const uint8_t *a, const uint8_t *b, size_t n) {
    uint8_t diff = 0;
    size_t i;
    for (i = 0; i < n; i++) {
        diff |= (uint8_t)(a[i] ^ b[i]);
    }
    return diff == 0;
}

/* ---------------- Random ---------------- */

static int random_bytes(uint8_t *buf, size_t n) {
    FILE *f = fopen("/dev/urandom", "rb");
    size_t r;
    if (!f) {
        return -1;
    }
    r = fread(buf, 1, n, f);
    fclose(f);
    return (r == n) ? 0 : -1;
}

/* ---------------- SHA-256 ---------------- */

typedef struct {
    uint32_t h[8];
    uint8_t buf[64];
    uint64_t total_len;
    size_t buf_len;
} sha256_ctx;

static const uint32_t K256[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

#define ROTR32(x, n) (((x) >> (n)) | ((x) << (32 - (n))))

static void sha256_init(sha256_ctx *c) {
    c->h[0] = 0x6a09e667; c->h[1] = 0xbb67ae85;
    c->h[2] = 0x3c6ef372; c->h[3] = 0xa54ff53a;
    c->h[4] = 0x510e527f; c->h[5] = 0x9b05688c;
    c->h[6] = 0x1f83d9ab; c->h[7] = 0x5be0cd19;
    c->total_len = 0;
    c->buf_len = 0;
}

static void sha256_transform(sha256_ctx *c, const uint8_t *block) {
    uint32_t w[64], a, b, cc, d, e, f, g, h, t1, t2;
    int i;

    for (i = 0; i < 16; i++) {
        w[i] = ((uint32_t)block[i * 4] << 24) |
               ((uint32_t)block[i * 4 + 1] << 16) |
               ((uint32_t)block[i * 4 + 2] << 8) |
               ((uint32_t)block[i * 4 + 3]);
    }
    for (i = 16; i < 64; i++) {
        uint32_t s0 = ROTR32(w[i - 15], 7) ^ ROTR32(w[i - 15], 18) ^ (w[i - 15] >> 3);
        uint32_t s1 = ROTR32(w[i - 2], 17) ^ ROTR32(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i