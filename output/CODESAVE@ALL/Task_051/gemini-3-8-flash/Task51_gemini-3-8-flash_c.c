#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void rc4_init(unsigned char *s, const unsigned char *key, size_t key_len) {
    size_t i, j = 0;
    unsigned char temp;

    for (i = 0; i < 256; i++) {
        s[i] = (unsigned char)i;
    }

    for (i = 0; i < 256; i++) {
        j = (j + s[i] + key[i % key_len]) % 256;
        temp = s[i];
        s[i] = s[j];
        s[j] = temp;
    }
}

void rc4_crypt(unsigned char *s, const unsigned char *input, unsigned char *output, size_t len) {
    size_t i = 0, j = 0, k;
    unsigned char temp;

    for (k = 0; k < len; k++) {
        i = (i + 1) % 256;
        j = (j + s[i]) % 256;
        temp = s[i];
        s[i] = s[j];
        s[j] = temp;
        output[k] = input[k] ^ s[(s[i] + s[j]) % 256];
    }
}

char *bytes_to_hex(const unsigned char *data, size_t len) {
    char *hex_str = (char *)malloc(len * 2 + 1);
    if (!hex_str) {
        return NULL;
    }

    for (size_t i = 0; i < len; i++) {
        sprintf(hex_str + (i * 2), "%02X", data[i]);
    }
    hex_str[len * 2] = '\0';
    return hex_str;
}

unsigned char *hex_to_bytes(const char *hex_str, size_t *out_len) {
    size_t hex_len = strlen(hex_str);
    if (hex_len % 2 != 0) {
        return NULL;
    }

    *out_len = hex_len / 2;
    unsigned char *data = (unsigned char *)malloc(*out_len);
    if (!data) {
        return NULL;
    }

    for (size_t i = 0; i < *out_len; i++) {
        unsigned int byte_val;
        if (sscanf(hex_str + (i * 2), "%02X", &byte_val) != 1) {
            free(data);
            return NULL;
        }
        data[i] = (unsigned char)byte_val;
    }

    return data;
}

char *rc4_encrypt_string(const char *plaintext, const char *key) {
    if (!plaintext || !key) {
        return NULL;
    }

    size_t len = strlen(plaintext);
    size_t key_len = strlen(key);
    if (key_len == 0) {
        return NULL;
    }

    unsigned char s[256];
    rc4_init(s, (const unsigned char *)key, key_len);

    unsigned char *ciphertext = (unsigned char *)malloc(len);
    if (!ciphertext) {
        return NULL;
    }

    rc4_crypt(s, (const unsigned char *)plaintext, ciphertext, len);

    char *hex_output = bytes_to_hex(ciphertext, len);
    free(ciphertext);

    return hex_output;
}

char *rc4_decrypt_string(const char *hex_ciphertext, const char *key) {
    if (!hex_ciphertext || !key) {
        return NULL;
    }

    size_t cipher_len = 0;
    unsigned char *ciphertext = hex_to_bytes(hex_ciphertext, &cipher_len);
    if (!ciphertext) {
        return NULL;
    }

    size_t key_len = strlen(key);
    if (key_len == 0) {
        free(ciphertext);
        return NULL;
    }

    unsigned char s[256];
    rc4_init(s, (const unsigned char *)key, key_len);

    char *plaintext = (char *)malloc(cipher_len + 1);
    if (!plaintext) {
        free(ciphertext);
        return NULL;
    }

    rc4_crypt(s, ciphertext, (unsigned char *)plaintext, cipher_len);
    plaintext[cipher_len] = '\0';

    free(ciphertext);
    return plaintext;
}

int main(void) {
    const char *plaintext = "Hello, World! This is a test of RC4 encryption in C.";
    const char *key = "SecretEncryptionKey";

    char *encrypted_hex = rc4_encrypt_string(plaintext, key);
    if (!encrypted_hex) {
        fprintf(stderr, "Encryption failed.\n");
        return 1;
    }

    char *decrypted_text = rc4_decrypt_string(encrypted_hex, key);
    if (!decrypted_text) {
        fprintf(stderr, "Decryption failed.\n");
        free(encrypted_hex);
        return 1;
    }

    printf("Original:  %s\n", plaintext);
    printf("Encrypted: %s\n", encrypted_hex);
    printf("Decrypted: %s\n", decrypted_text);

    free(encrypted_hex);
    free(decrypted_text);

    return 0;
}