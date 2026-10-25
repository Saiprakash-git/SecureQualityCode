#include <stdio.h>
#include <string.h>
#include <stdint.h>

static void xtea_encrypt_block(uint32_t v[2], const uint32_t key[4]) {
    uint32_t v0 = v[0], v1 = v[1], sum = 0;
    const uint32_t delta = 0x9E3779B9;
    for (int i = 0; i < 32; i++) {
        v0 += (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
        sum += delta;
        v1 += (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum >> 11) & 3]);
    }
    v[0] = v0;
    v[1] = v1;
}

static void xtea_decrypt_block(uint32_t v[2], const uint32_t key[4]) {
    uint32_t v0 = v[0], v1 = v[1];
    const uint32_t delta = 0x9E3779B9;
    uint32_t sum = delta * 32;
    for (int i = 0; i < 32; i++) {
        v1 -= (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum >> 11) & 3]);
        sum -= delta;
        v0 -= (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
    }
    v[0] = v0;
    v[1] = v1;
}

static size_t xtea_encrypt_string(const unsigned char *in, size_t len,
                                  unsigned char *out, const uint32_t key[4]) {
    size_t blocks = (len + 8) / 8 + ((len + 8) % 8 ? 1 : 0);
    size_t total = blocks * 8;
    unsigned char *buf = out;
    memset(buf, 0, total);
    memcpy(buf, in, len);
    for (size_t b = 0; b < blocks; b++) {
        uint32_t v[2];
        v[0] = (uint32_t)buf[b*8+0] | ((uint32_t)buf[b*8+1] << 8) |
               ((uint32_t)buf[b*8+2] << 16) | ((uint32_t)buf[b*8+3] << 24);
        v[1] = (uint32_t)buf[b*8+4] | ((uint32_t)buf[b*8+5] << 8) |
               ((uint32_t)buf[b*8+6] << 16) | ((uint32_t)buf[b*8+7] << 24);
        xtea_encrypt_block(v, key);
        for (int i = 0; i < 4; i++) {
            buf[b*8+i]   = (unsigned char)(v[0] >> (8*i));
            buf[b*8+4+i] = (unsigned char)(v[1] >> (8*i));
        }
    }
    return total;
}

static void xtea_decrypt_string(const unsigned char *in, size_t len,
                                unsigned char *out, const uint32_t key[4]) {
    memcpy(out, in, len);
    for (size_t b = 0; b < len / 8; b++) {
        uint32_t v[2];
        v[0] = (uint32_t)out[b*8+0] | ((uint32_t)out[b*8+1] << 8) |
               ((uint32_t)out[b*8+2] << 16) | ((uint32_t)out[b*8+3] << 24);
        v[1] = (uint32_t)out[b*8+4] | ((uint32_t)out[b*8+5] << 8) |
               ((uint32_t)out[b*8+6] << 16) | ((uint32_t)out[b*8+7] << 24);
        xtea_decrypt_block(v, key);
        for (int i = 0; i < 4; i++) {
            out[b*8+i]   = (unsigned char)(v[0] >> (8*i));
            out[b*8+4+i] = (unsigned char)(v[1] >> (8*i));
        }
    }
}

int main(void) {
    const char *text = "Hello, World! This is a secret message.";
    const uint32_t key[4] = {0x01234567, 0x89ABCDEF, 0xFEDCBA98, 0x76543210};
    unsigned char cipher[256];
    unsigned char plain[256];
    size_t len = strlen(text);

    size_t clen = xtea_encrypt_string((const unsigned char *)text, len, cipher, key);

    printf("Original:  %s\n", text);
    printf("Encrypted (hex): ");
    for (size_t i = 0; i < clen; i++) printf("%02x", cipher[i]);
    printf("\n");

    xtea_decrypt_string(cipher, clen, plain, key);
    plain[len] = '\0';
    printf("Decrypted: %s\n", plain);

    return 0;
}