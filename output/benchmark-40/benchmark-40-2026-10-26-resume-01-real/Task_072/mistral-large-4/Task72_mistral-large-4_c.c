#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>

unsigned char *encrypt_aes_cbc(const unsigned char *plaintext, int plaintext_len,
                               const unsigned char *key, int *out_len) {
    static const unsigned char iv[16] = {
        0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08,
        0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00
    };

    EVP_CIPHER_CTX *ctx = NULL;
    unsigned char *ciphertext = NULL;
    int len = 0;
    int ciphertext_len = 0;

    ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return NULL;

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return NULL;
    }

    ciphertext = (unsigned char *)malloc(plaintext_len + EVP_MAX_BLOCK_LENGTH);
    if (!ciphertext) {
        EVP_CIPHER_CTX_free(ctx);
        return NULL;
    }

    if (EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len) != 1) {
        free(ciphertext);
        EVP_CIPHER_CTX_free(ctx);
        return NULL;
    }
    ciphertext_len = len;

    if (EVP_EncryptFinal_ex(ctx, ciphertext + len, &len) != 1) {
        free(ciphertext);
        EVP_CIPHER_CTX_free(ctx);
        return NULL;
    }
    ciphertext_len += len;

    EVP_CIPHER_CTX_free(ctx);
    *out_len = ciphertext_len;
    return ciphertext;
}

int main(void) {
    unsigned char key[32];
    unsigned char plaintext[] = "This is a secret message that needs to be encrypted using AES CBC mode.";
    unsigned char *ciphertext;
    int ciphertext_len;
    int i;

    for (i = 0; i < 32; i++) {
        key[i] = i;
    }

    ciphertext = encrypt_aes_cbc(plaintext, strlen((char *)plaintext), key, &ciphertext_len);

    if (ciphertext == NULL) {
        fprintf(stderr, "Encryption failed\n");
        return 1;
    }

    printf("Ciphertext length: %d\n", ciphertext_len);
    printf("Ciphertext (hex): ");
    for (i = 0; i < ciphertext_len; i++) {
        printf("%02x", ciphertext[i]);
    }
    printf("\n");

    free(ciphertext);
    return 0;
}