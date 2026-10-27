#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX_RECORDS 64
#define KEY_SIZE 32
#define NONCE_SIZE 12
#define MAX_ID_LEN 32

#define ROTL32(v, c) (((v) << (c)) | ((v) >> (32 - (c))))

#define CHACHA20_QUARTERROUND(a, b, c, d) \
    do { \
        a += b; d ^= a; d = ROTL32(d, 16); \
        c += d; b ^= c; b = ROTL32(b, 12); \
        a += b; d ^= a; d = ROTL32(d, 8);  \
        c += d; b ^= c; b = ROTL32(b, 7);  \
    } while (0)

typedef struct {
    char id[MAX_ID_LEN];
    uint8_t *ciphertext;
    size_t cipher_len;
    uint8_t nonce[NONCE_SIZE];
} EncryptedRecord;

typedef struct {
    EncryptedRecord records[MAX_RECORDS];
    size_t record_count;
    uint8_t master_key[KEY_SIZE];
} SecureVault;

void secure_wipe(void *v, size_t n) {
    volatile uint8_t *p = (volatile uint8_t *)v;
    while (n--) {
        *p++ = 0;
    }
}

static void chacha20_block(uint32_t out[16], const uint32_t in[16]) {
    int i;
    uint32_t x[16];

    for (i = 0; i < 16; ++i) {
        x[i] = in[i];
    }

    for (i = 0; i < 10; ++i) {
        CHACHA20_QUARTERROUND(x[0], x[4], x[8],  x[12]);
        CHACHA20_QUARTERROUND(x[1], x[5], x[9],  x[13]);
        CHACHA20_QUARTERROUND(x[2], x[6], x[10], x[14]);
        CHACHA20_QUARTERROUND(x[3], x[7], x[11], x[15]);
        CHACHA20_QUARTERROUND(x[0], x[5], x[10], x[15]);
        CHACHA20_QUARTERROUND(x[1], x[6], x[11], x[12]);
        CHACHA20_QUARTERROUND(x[2], x[7], x[8],  x[13]);
        CHACHA20_QUARTERROUND(x[3], x[4], x[9],  x[14]);
    }

    for (i = 0; i < 16; ++i) {
        out[i] = x[i] + in[i];
    }
}

static uint32_t load32_le(const uint8_t *src) {
    return (uint32_t)src[0] |
          ((uint32_t)src[1] << 8) |
          ((uint32_t)src[2] << 16) |
          ((uint32_t)src[3] << 24);
}

static void store32_le(uint8_t *dst, uint32_t val) {
    dst[0] = (uint8_t)(val & 0xFF);
    dst[1] = (uint8_t)((val >> 8) & 0xFF);
    dst[2] = (uint8_t)((val >> 16) & 0xFF);
    dst[3] = (uint8_t)((val >> 24) & 0xFF);
}

int chacha20_crypt(const uint8_t key[KEY_SIZE], const uint8_t nonce[NONCE_SIZE],
                   uint32_t counter, const uint8_t *input, uint8_t *output,
                   size_t length) {
    uint32_t state[16];
    uint32_t block[16];
    uint8_t stream[64];
    size_t i, curr_len;

    if (!key || !nonce || !input || !output) {
        return -1;
    }

    state[0] = 0x61707865;
    state[1] = 0x3320646e;
    state[2] = 0x79622d32;
    state[3] = 0x6b206574;

    state[4] = load32_le(key + 0);
    state[5] = load32_le(key + 4);
    state[6] = load32_le(key + 8);
    state[7] = load32_le(key + 12);
    state[8] = load32_le(key + 16);
    state[9] = load32_le(key + 20);
    state[10] = load32_le(key + 24);
    state[11] = load32_le(key + 28);

    state[12] = counter;
    state[13] = load32_le(nonce + 0);
    state[14] = load32_le(nonce + 4);
    state[15] = load32_le(nonce + 8);

    while (length > 0) {
        chacha20_block(block, state);
        state[12]++;

        for (i = 0; i < 16; ++i) {
            store32_le(stream + (i * 4), block[i]);
        }

        curr_len = (length < 64) ? length : 64;
        for (i = 0; i < curr_len; ++i) {
            *output++ = *input++ ^ stream[i];
        }

        length -= curr_len;
    }

    secure_wipe(state, sizeof(state));
    secure_wipe(block, sizeof(block));
    secure_wipe(stream, sizeof(stream));

    return 0;
}

int vault_init(SecureVault *vault, const uint8_t key[KEY_SIZE]) {
    if (!vault || !key) {
        return -1;
    }
    memcpy(vault->master_key, key, KEY_SIZE);
    vault->record_count = 0;
    memset(vault->records, 0, sizeof(vault->records));
    return 0;
}

int vault_generate_nonce(uint8_t nonce[NONCE_SIZE]) {
    FILE *f = fopen("/dev/urandom", "rb");
    if (!f) {
        for (int i = 0; i < NONCE_SIZE; ++i) {
            nonce[i] = (uint8_t)(rand() & 0xFF);
        }
        return 0;
    }
    if (fread(nonce, 1, NONCE_SIZE, f) != NONCE_SIZE) {
        fclose(f);
        return -1;
    }
    fclose(f);
    return 0;
}

int vault_store_record(SecureVault *vault, const char *record_id,
                       const uint8_t *plaintext, size_t plain_len) {
    if (!vault || !record_id || !plaintext || plain_len == 0) {
        return -1;
    }
    if (vault->record_count >= MAX_RECORDS) {
        return -2;
    }

    for (size_t i = 0; i < vault->record_count; ++i) {
        if (strncmp(vault->records[i].id, record_id, MAX_ID_LEN) == 0) {
            return -3;
        }
    }

    EncryptedRecord *rec = &vault->records[vault->record_count];
    strncpy(rec->id, record_id, MAX_ID_LEN - 1);
    rec->id[MAX_ID_LEN - 1] = '\0';

    if (vault_generate_nonce(rec->nonce) != 0) {
        return -4;
    }

    rec->ciphertext = (uint8_t *)malloc(plain_len);
    if (!rec->ciphertext) {
        return -5;
    }
    rec->cipher_len = plain_len;

    if (chacha20_crypt(vault->master_key, rec->nonce, 1, plaintext,
                       rec->ciphertext, plain_len) != 0) {
        free(rec->ciphertext);
        rec->ciphertext = NULL;
        return -6;
    }

    vault->record_count++;
    return 0;
}

int vault_retrieve_record(SecureVault *vault, const char *record_id,
                          uint8_t **out_plaintext, size_t *out_plain_len) {
    if (!vault || !record_id || !out_plaintext || !out_plain_len) {
        return -1;
    }

    for (size_t i = 0; i < vault->record_count; ++i) {
        EncryptedRecord *rec = &vault->records[i];
        if (strncmp(rec->id, record_id, MAX_ID_LEN) == 0) {
            uint8_t *decrypted = (uint8_t *)malloc(rec->cipher_len + 1);
            if (!decrypted) {
                return -2;
            }

            if (chacha20_crypt(vault->master_key, rec->nonce, 1, rec->ciphertext,
                               decrypted, rec->cipher_len) != 0) {
                free(decrypted);
                return -3;
            }

            decrypted[rec->cipher_len] = '\0';
            *out_plaintext = decrypted;
            *out_plain_len = rec->cipher_len;
            return 0;
        }
    }

    return -4;
}

void vault_close(SecureVault *vault) {
    if (!vault) {
        return;
    }
    for (size_t i = 0; i < vault->record_count; ++i) {
        if (vault->records[i].ciphertext) {
            secure_wipe(vault->records[i].ciphertext, vault->records[i].cipher_len);
            free(vault->records[i].ciphertext);
            vault->records[i].ciphertext = NULL;
        }
    }
    secure_wipe(vault->master_key, KEY_SIZE);
    vault->record_count = 0;
}

int main(void) {
    uint8_t master_key[KEY_SIZE] = {
        0x10, 0x32, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe,
        0xef, 0xcd, 0xab, 0x89, 0x67, 0x45, 0x23, 0x01,
        0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
        0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10
    };

    SecureVault vault;
    if (vault_init(&vault, master_key) != 0) {
        fprintf(stderr, "Vault initialization failed.\n");
        return 1;
    }
    secure_wipe(master_key, KEY_SIZE);

    const char *cc_data = "CardNumber=4111222233334444;Exp=12/28;CVV=987";
    const char *pii_data = "Name=John Doe;SSN=000-12-3456;DOB=1985-04-12";

    if (vault_store_record(&vault, "credit_card", (const uint8_t *)cc_data, strlen(cc_data)) != 0) {
        fprintf(stderr, "Failed to encrypt and store credit card record.\n");
        vault_close(&vault);
        return 1;
    }

    if (vault_store_record(&vault, "