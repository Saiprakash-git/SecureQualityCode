#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>

static const unsigned char STATIC_IV[16] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
};

static int hex_value(int c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

static int parse_key(const char *hex, unsigned char key[32])
{
    size_t i;

    if (hex == NULL || strlen(hex) != 64)
        return 0;

    for (i = 0; i < 32; ++i) {
        int high = hex_value((unsigned char)hex[i * 2]);
        int low = hex_value((unsigned char)hex[i * 2 + 1]);

        if (high < 0 || low < 0)
            return 0;
        key[i] = (unsigned char)((high << 4) | low);
    }

    return 1;
}

static int read_stdin(unsigned char **data, size_t *data_len)
{
    size_t capacity = 4096;
    size_t length = 0;
    unsigned char *buffer = malloc(capacity);

    if (buffer == NULL)
        return 0;

    for (;;) {
        size_t count;

        if (length == capacity) {
            size_t new_capacity;
            unsigned char *new_buffer;

            if (capacity > SIZE_MAX / 2) {
                free(buffer);
                return 0;
            }

            new_capacity = capacity * 2;
            new_buffer = realloc(buffer, new_capacity);
            if (new_buffer == NULL) {
                free(buffer);
                return 0;
            }

            buffer = new_buffer;
            capacity = new_capacity;
        }

        count = fread(buffer + length, 1, capacity - length, stdin);
        length += count;

        if (count == 0) {
            if (ferror(stdin)) {
                free(buffer);
                return 0;
            }
            break;
        }
    }

    *data = buffer;
    *data_len = length;
    return 1;
}

static int encrypt_data(const unsigned char *plaintext, size_t plaintext_len,
                        const unsigned char key[32],
                        unsigned char **ciphertext, size_t *ciphertext_len)
{
    EVP_CIPHER_CTX *ctx = NULL;
    unsigned char *output = NULL;
    unsigned char empty_input = 0;
    int update_len = 0;
    int final_len = 0;
    int ok = 0;

    if (ciphertext == NULL || ciphertext_len == NULL || key == NULL ||
        (plaintext == NULL && plaintext_len != 0))
        return 0;

    *ciphertext = NULL;
    *ciphertext_len = 0;

    if (plaintext_len > INT_MAX ||
        plaintext_len > SIZE_MAX - EVP_MAX_BLOCK_LENGTH)
        return 0;

    output = OPENSSL_malloc(plaintext_len + EVP_MAX_BLOCK_LENGTH);
    if (output == NULL)
        return 0;

    ctx = EVP_CIPHER_CTX_new();
    if (ctx == NULL)
        goto cleanup;

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, STATIC_IV) != 1)
        goto cleanup;

    if (EVP_EncryptUpdate(ctx, output, &update_len,
                          plaintext_len == 0 ? &empty_input : plaintext,
                          (int)plaintext_len) != 1)
        goto cleanup;

    if (EVP_EncryptFinal_ex(ctx, output + update_len, &final_len) != 1)
        goto cleanup;

    *ciphertext = output;
    *ciphertext_len = (size_t)update_len + (size_t)final_len;
    output = NULL;
    ok = 1;

cleanup:
    EVP_CIPHER_CTX_free(ctx);
    OPENSSL_free(output);
    return ok;
}

int main(int argc, char **argv)
{
    unsigned char key[32];
    unsigned char *plaintext = NULL;
    unsigned char *ciphertext = NULL;
    size_t plaintext_len = 0;
    size_t ciphertext_len = 0;
    int result = EXIT_FAILURE;

    if (argc != 2 || !parse_key(argv[1], key)) {
        fprintf(stderr, "Usage: %s <64-character-hex-key>\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (!read_stdin(&plaintext, &plaintext_len)) {
        fprintf(stderr, "Failed to read input\n");
        goto cleanup;
    }

    if (!encrypt_data(plaintext, plaintext_len, key,
                      &ciphertext, &ciphertext_len)) {
        fprintf(stderr, "Encryption failed\n");
        goto cleanup;
    }

    if (fwrite(ciphertext, 1, ciphertext_len, stdout) != ciphertext_len) {
        fprintf(stderr, "Failed to write encrypted output\n");
        goto cleanup;
    }

    result = EXIT_SUCCESS;

cleanup:
    free(plaintext);
    OPENSSL_free(ciphertext);
    return result;
}