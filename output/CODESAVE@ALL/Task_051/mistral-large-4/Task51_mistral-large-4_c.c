#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char* encrypt(const char* plaintext, const char* key) {
    if (plaintext == NULL || key == NULL) {
        return NULL;
    }

    size_t len = strlen(plaintext);
    size_t key_len = strlen(key);
    char* ciphertext = (char*)malloc(len + 1);

    if (ciphertext == NULL) {
        return NULL;
    }

    for (size_t i = 0; i < len; i++) {
        ciphertext[i] = plaintext[i] ^ key[i % key_len];
    }
    ciphertext[len] = '\0';

    return ciphertext;
}

char* decrypt(const char* ciphertext, const char* key) {
    return encrypt(ciphertext, key);
}

int main(int argc, char* argv[]) {
    const char* input = (argc > 1) ? argv[1] : "Hello, World!";
    const char* key = (argc > 2) ? argv[2] : "secretkey";

    char* encrypted = encrypt(input, key);
    if (encrypted == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    printf("Plaintext: %s\n", input);
    printf("Key: %s\n", key);
    printf("Encrypted (hex): ");

    for (size_t i = 0; i < strlen(encrypted); i++) {
        printf("%02x", (unsigned char)encrypted[i]);
    }
    printf("\n");

    char* decrypted = decrypt(encrypted, key);
    if (decrypted != NULL) {
        printf("Decrypted: %s\n", decrypted);
        free(decrypted);
    }

    free(encrypted);
    return 0;
}