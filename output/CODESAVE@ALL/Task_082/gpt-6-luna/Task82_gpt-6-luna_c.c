#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#define HASH_SIZE 32
#define DEFAULT_ITERATIONS 600000U

typedef struct {
    uint8_t data[64];
    uint32_t state[8];
    uint64_t bitlen;
    size_t datalen;
} Sha256Context;

static const uint32_t sha256_constants[64] = {
    0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U,
    0x3956c25bU, 0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U,
    0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U,
    0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U,
    0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU,
    0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
    0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U,
    0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U,
    0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U,
    0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
    0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U,
    0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
    0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U,
    0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
    0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U,
    0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U
};

static uint32_t rotate_right(uint32_t value, unsigned int count)
{
    return (value >> count) | (value << (32U - count));
}

static void sha256_transform(Sha256Context *context, const uint8_t block[64])
{
    uint32_t words[64];
    uint32_t a, b, c, d, e, f, g, h;
    size_t i;

    for (i = 0; i < 16; ++i) {
        words[i] = ((uint32_t)block[i * 4] << 24) |
                   ((uint32_t)block[i * 4 + 1] << 16) |
                   ((uint32_t)block[i * 4 + 2] << 8) |
                   (uint32_t)block[i * 4 + 3];
    }

    for (i = 16; i < 64; ++i) {
        uint32_t s0 = rotate_right(words[i - 15], 7) ^
                      rotate_right(words[i - 15], 18) ^
                      (words[i - 15] >> 3);
        uint32_t s1 = rotate_right(words[i - 2], 17) ^
                      rotate_right(words[i - 2], 19) ^
                      (words[i - 2] >> 10);
        words[i] = words[i - 16] + s0 + words[i - 7] + s1;
    }

    a = context->state[0];
    b = context->state[1];
    c = context->state[2];
    d = context->state[3];
    e = context->state[4];
    f = context->state[5];
    g = context->state[6];
    h = context->state[7];

    for (i = 0; i < 64; ++i) {
        uint32_t sum1 = rotate_right(e, 6) ^ rotate_right(e, 11) ^
                        rotate_right(e, 25);
        uint32_t choice = (e & f) ^ (~e & g);
        uint32_t temp1 = h + sum1 + choice + sha256_constants[i] + words[i];
        uint32_t sum0 = rotate_right(a, 2) ^ rotate_right(a, 13) ^
                        rotate_right(a, 22);
        uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
        uint32_t temp2 = sum0 + majority;

        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
    }

    context->state[0] += a;
    context->state[1] += b;
    context->state[2] += c;
    context->state[3] += d;
    context->state[4] += e;
    context->state[5] += f;
    context->state[6] += g;
    context->state[7] += h;
}

static void sha256_init(Sha256Context *context)
{
    context->datalen = 0;
    context->bitlen = 0;
    context->state[0] = 0x6a09e667U;
    context->state[1] = 0xbb67ae85U;
    context->state[2] = 0x3c6ef372U;
    context->state[3] = 0xa54ff53aU;
    context->state[4] = 0x510e527fU;
    context->state[5] = 0x9b05688cU;
    context->state[6] = 0x1f83d9abU;
    context->state[7] = 0x5be0cd19U;
}

static void sha256_update(Sha256Context *context, const uint8_t *data, size_t length)
{
    size_t i;

    for (i = 0; i < length; ++i) {
        context->data[context->datalen++] = data[i];
        if (context->datalen == 64) {
            sha256_transform(context, context->data);
            context->bitlen += 512;
            context->datalen = 0;
        }
    }
}

static void sha256_final(Sha256Context *context, uint8_t hash[HASH_SIZE])
{
    size_t i = context->datalen;
    uint64_t bitlen = context->bitlen + (uint64_t)context->datalen * 8U;

    context->data[i++] = 0x80;

    if (i > 56) {
        while (i < 64)
            context->data[i++] = 0;
        sha256_transform(context, context->data);
        i = 0;
    }

    while (i < 56)
        context->data[i++] = 0;

    for (i = 0; i < 8; ++i)
        context->data[63 - i] = (uint8_t)(bitlen >> (i * 8));

    sha256_transform(context, context->data);

    for (i = 0; i < 8; ++i) {
        hash[i * 4] = (uint8_t)(context->state[i] >> 24);
        hash[i * 4 + 1] = (uint8_t)(context->state[i] >> 16);
        hash[i * 4 + 2] = (uint8_t)(context->state[i] >> 8);
        hash[i * 4 + 3] = (uint8_t)context->state[i];
    }
}

static void hmac_sha256(const uint8_t *key, size_t key_length,
                        const uint8_t *data, size_t data_length,
                        uint8_t output[HASH_SIZE])
{
    uint8_t key_block[64] = {0};
    uint8_t inner_hash[HASH_SIZE];
    uint8_t pad[64];
    Sha256Context context;
    size_t i;

    if (key_length > sizeof key_block) {
        sha256_init(&context);
        sha256_update(&context, key, key_length);
        sha256_final(&context, key_block);
    } else if (key_length != 0) {
        memcpy(key_block, key, key_length);
    }

    for (i = 0; i < sizeof pad; ++i)
        pad[i] = (uint8_t)(key_block[i] ^ 0x36U);

    sha256_init(&context);
    sha256_update(&context, pad, sizeof pad);
    sha256_update(&context, data, data_length);
    sha256_final(&context, inner_hash);

    for (i = 0; i < sizeof pad; ++i)
        pad[i] = (uint8_t)(key_block[i] ^ 0x5cU);

    sha256_init(&context);
    sha256_update(&context, pad, sizeof pad);
    sha256_update(&context, inner_hash, sizeof inner_hash);
    sha256_final(&context, output);
}

static int pbkdf2_hmac_sha256(const uint8_t *password, size_t password_length,
                              const uint8_t *salt, size_t salt_length,
                              uint32_t iterations, uint8_t output[HASH_SIZE])
{
    uint8_t *salt_block;
    uint8_t u[HASH_SIZE];
    uint8_t result[HASH_SIZE];
    size_t i;
    uint32_t round;

    if (iterations == 0 || salt_length > SIZE_MAX - 4)
        return 0;

    salt_block = (uint8_t *)malloc(salt_length + 4);
    if (salt_block == NULL)
        return 0;

    if (salt_length != 0)
        memcpy(salt_block, salt, salt_length);

    salt_block[salt_length] = 0;
    salt_block[salt_length + 1] = 0;
    salt_block[salt_length + 2] = 0;
    salt_block[salt_length + 3] = 1;

    hmac_sha256(password, password_length, salt_block, salt_length + 4, u);
    memcpy(result, u, sizeof result);

    for (round = 1; round < iterations; ++round) {
        hmac_sha256(password, password_length, u, sizeof u, u);
        for (i = 0; i < sizeof result; ++i)
            result[i] ^= u[i];
    }

    memcpy(output, result, sizeof result);
    memset(u, 0, sizeof u);
    memset(result, 0, sizeof result);
    memset(salt_block, 0, salt_length + 4);
    free(salt_block);
    return 1;
}

int password_hash(const uint8_t *password, size_t password_length,
                  const uint8_t *salt, size_t salt_length,
                  uint32_t iterations, char *hex_output, size_t output_size)
{
    static const char hex_digits[] = "0123456789abcdef";
    uint8_t hash[HASH_SIZE];
    size_t i;

    if (hex_output == NULL || output_size < HASH_SIZE * 2 + 1 ||
        (password == NULL && password_length != 0) ||
        (salt == NULL && salt_length != 0))
        return 0;

    if (!pbkdf2_hmac_sha256(password, password_length, salt, salt_length,
                            iterations, hash))
        return 0;

    for (i = 0; i < HASH_SIZE; ++i) {
        hex_output[i * 2] = hex_digits[hash[i] >> 4];
        hex_output[i * 2 + 1] = hex_digits[hash[i] & 0x0f];
    }
    hex_output[HASH_SIZE * 2] = '\0';

    memset(hash, 0, sizeof hash);
    return 1;
}

int main(int argc, char **argv)
{
    uint32_t iterations = DEFAULT_ITERATIONS;
    char hash[HASH_SIZE * 2 + 1];

    if (argc != 3 && argc != 4) {
        fprintf(stderr, "Usage: %s PASSWORD SALT [ITERATIONS]\n", argv[0]);
        return 2;
    }

    if (argc == 4) {
        char *end = NULL;
        unsigned long value;

        errno = 0;
        value = strtoul(argv[3], &end, 10);
        if (errno != 0 || end == argv[3] || *end != '\0' ||
            value == 0 || value > UINT32_MAX) {
            fprintf(stderr, "Invalid iteration count\n");
            return 2;
        }
        iterations = (uint32_t)value;
    }