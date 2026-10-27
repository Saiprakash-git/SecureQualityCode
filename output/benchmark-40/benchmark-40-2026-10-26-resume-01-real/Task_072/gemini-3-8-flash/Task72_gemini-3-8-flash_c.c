#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>

#define AES_BLOCK_SIZE 16

static const unsigned char STATIC_IV[AES_BLOCK_SIZE] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};

unsigned char *pkcs7_pad(const unsigned char *data, size_t data_len, size_t *padded_len) {
    size_t pad_len = AES_BLOCK_SIZE - (data_len % AES_BLOCK_SIZE);
    *padded_len = data_len + pad_len;

    unsigned char *padded_data = (unsigned char *)malloc(*padded_len);
    if (!padded_data) {
        return NULL;
    }

    memcpy(padded_data, data, data_len);
    memset(padded_data + data_len, (int)pad_len, pad_len);

    return padded_data;
}

unsigned char *encrypt_aes_cbc(const unsigned char *plaintext, size_t plaintext_len, const unsigned char *key, size_t *ciphertext_len) {
    if (!plaintext || !key || !ciphertext_len) {
        return NULL;
    }

    size_t padded_len = 0;
    unsigned char *padded_data = pkcs7_pad(plaintext, plaintext_len, &padded_len);
    if (!padded_data) {
        return NULL;
    }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        free(padded_data);
        return NULL;
    }

    if (EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, STATIC_IV) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        free(padded_data);
        return NULL;
    }

    EVP_CIPHER_CTX_set_padding(ctx, 0);

    unsigned char *ciphertext = (unsigned char *)malloc(padded_len);
    if (!ciphertext) {
        EVP_CIPHER_CTX_free(ctx);
        free(padded_data);
        return NULL;
    }

    int len = 0;
    int total_len = 0;

    if (EVP_EncryptUpdate(ctx, ciphertext, &len, padded_data, (int)padded_len) != 1) {
        free(ciphertext);
        EVP_CIPHER_CTX_free(ctx);
        free(padded_data);
        return NULL;
    }
    total_len += len;

    if (EVP_EncryptFinal_ex(ctx, ciphertext + total_len, &len) != 1) {
        free(ciphertext);
        EVP_CIPHER_CTX_free(ctx);
        free(padded_data);
        return NULL;
    }
    total_len += len;

    *ciphertext_len = (size_t)total_len;

    EVP_CIPHER_CTX_free(ctx);
    free(padded_data);

    return ciphertext;
}

int main(void) {
    const unsigned char key[AES_BLOCK_SIZE] = "0123456789abcdef";
    const unsigned char plaintext[] = "Data to encrypt using AES-CBC with static IV.";
    size_t plaintext_len = strlen((const char *)plaintext);
    size_t ciphertext_len = 0;

    unsigned char *ciphertext = encrypt_aes_cbc(plaintext, plaintext_len, key, &ciphertext_len);

    if (ciphertext) {
        printf("Plaintext: %s\n", plaintext);
        printf("Ciphertext (hex): ");
        for (size_t i = 0; i < ciphertext_len; i++) {
            printf("%02x", ciphertext[i]);
        }
        printf("\n");
        free(ciphertext);
    } else {
        fprintf(stderr, "Encryption failed.\n");
        return 1;
    }

    return 0;
}