```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define AES_BLOCK_SIZE 16
#define AES128_ROUNDS 10
#define AES128_KEY_SIZE 16
#define AES128_EXPANDED_SIZE (16 * (AES128_ROUNDS + 1))

/* Static initialization vector */
static const uint8_t STATIC_IV[AES_BLOCK_SIZE] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};

static uint8_t xtime(uint8_t x)
{
    return (uint8_t)(((x << 1) ^ ((x & 0x80) ? 0x1B : 0x00)) & 0xFF);
}

static uint8_t gmul(uint8_t a, uint8_t b)
{
    uint8_t p = 0;
    while (b) {
        if (b & 1)
            p ^= a;
        a = xtime(a);
        b >>= 1;
    }
    return p;
}

static uint8_t gf_inverse(uint8_t x)
{
    if (x == 0)
        return 0;
    for (int y = 1; y < 256; y++) {
        if (gmul(x, (uint8_t)y) == 1)
            return (uint8_t)y;
    }
    return 0;
}

static uint8_t rotl8(uint8_t x, int n)
{
    return (uint8_t)(((x << n) | (x >> (8 - n))) & 0xFF);
}

static void build_sbox(uint8_t sbox[256])
{
    for (int i = 0; i < 256; i++) {
        uint8_t b = gf_inverse((uint8_t)i);
        sbox[i] = (uint8_t)(b ^ rotl8(b, 1) ^ rotl8(b, 2) ^
                            rotl8(b, 3) ^ rotl8(b, 4) ^ 0x63);
    }
}

/* Initialize the cipher: expand the 128-bit key into round keys */
static void aes128_key_expand(const uint8_t key[AES128_KEY_SIZE],
                              const uint8_t sbox[256],
                              uint8_t rk[AES128_EXPANDED_SIZE])
{
    uint8_t rcon = 1;
    memcpy(rk, key, AES128_KEY_SIZE);

    for (int i = AES128_KEY_SIZE; i < AES128_EXPANDED_SIZE; i += 4) {
        uint8_t t[4];
        memcpy(t, rk + i - 4, 4);
        if (i % AES128_KEY_SIZE == 0) {
            uint8_t first = t[0];
            t[0] = sbox[t[1]];
            t[1] = sbox[t[2]];
            t[2] = sbox[t[3]];
            t[3] = sbox[first];
            t[0] ^= rcon;
            rcon = xtime(rcon);
        }
        for (int k = 0; k < 4; k++)
            rk[i + k] = rk[i - AES128_KEY_SIZE + k] ^ t[k];
    }
}

static void aes128_encrypt_block(const uint8_t in[AES_BLOCK_SIZE],
                                 uint8_t out[AES_BLOCK_SIZE],
                                 const uint8_t sbox[256],
                                 const uint8_t rk[AES128_EXPANDED_SIZE])
{
    uint8_t s[16], tmp[16];

    for (int i = 0; i < 16; i++)
        s[i] = in[i] ^ rk[i];

    for (int round = 1; round <= AES128_ROUNDS; round++) {
        /* SubBytes */
        for (int i = 0; i < 16; i++)
            s[i] = sbox[s[i]];

        /* ShiftRows: state index = r + 4*c */
        for (int r = 0; r < 4; r++)
            for (int c = 0; c < 4; c++)
                tmp[r + 4 * c] = s[r + 4 * ((c + r) % 4)];
        memcpy(s, tmp, 16);

        /* MixColumns (skipped in final round) */
        if (round != AES128_ROUNDS) {
            for (int c = 0; c < 4; c++) {
                uint8_t *a = s + 4 * c;
                uint8_t a0 = a[0], a1 = a[1], a2 = a[2], a3 = a[3];
                a[0] = gmul(a0, 2) ^ gmul(a1, 3) ^ a2 ^ a3;
                a[1] = a0 ^ gmul(a1, 2) ^ gmul(a2, 3) ^ a3;
                a[2] = a0 ^ a1 ^ gmul(a2, 2) ^ gmul(a3, 3);
                a[3] = gmul(a0, 3) ^ a1 ^ a2 ^ gmul(a3, 2);
            }
        }

        /* AddRoundKey */
        for (int i = 0; i < 16; i++)
            s[i] ^= rk[round * 16 + i];
    }

    memcpy(out, s, 16);
}

/*
 * Pads input with PKCS#7, encrypts it with AES-128 in CBC mode using the
 * static IV, and returns a newly allocated buffer containing the ciphertext.
 * The ciphertext length is stored in *out_len. Returns NULL on failure.
 * The caller must free the returned buffer.
 */
uint8_t *aes_cbc_encrypt_data(const uint8_t *data, size_t len,
                              const uint8_t key[AES128_KEY_SIZE],
                              size_t *out_len)
{
    uint8_t sbox[256];
    uint8_t rk[AES128_EXPANDED_SIZE];
    uint8_t prev[AES_BLOCK_SIZE];
    uint8_t block[AES_BLOCK_SIZE];
    uint8_t enc[AES_BLOCK_SIZE];

    if (!data && len > 0)
        return NULL;
    if (!key || !out_len)
        return NULL;

    /* Pad input data to block size (PKCS#7) */
    size_t pad = AES_BLOCK_SIZE - (len % AES_BLOCK_SIZE);
    size_t padded_len = len + pad;

    uint8_t *padded = malloc(padded_len);
    if (!padded)
        return NULL;
    if (len > 0)
        memcpy(padded, data, len);
    memset(padded + len, (int)pad, pad);

    uint8_t *out = malloc(padded_len);
    if (!out) {
        free(padded);
        return NULL;
    }

    /* Initialize cipher */
    build_sbox(sbox);
    aes128_key_expand(key, sbox, rk);

    /* CBC encryption */
    memcpy(prev, STATIC_IV, AES_BLOCK_SIZE);
    for (size_t off = 0; off < padded_len; off += AES_BLOCK_SIZE) {
        for (int i = 0; i < AES_BLOCK_SIZE; i++)
            block[i] = padded[off + i] ^ prev[i];
        aes128_encrypt_block(block, enc, sbox, rk);
        memcpy(out +