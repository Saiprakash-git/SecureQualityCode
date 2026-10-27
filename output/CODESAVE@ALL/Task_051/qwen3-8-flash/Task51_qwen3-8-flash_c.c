#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

char *encrypt_string(const char *plaintext, const char *key)
{
    size_t len;
    size_t key_len;
    size_t i;
    size_t j;
    char *result;

    if (plaintext == NULL || key == NULL) {
        return NULL;
    }

    key_len = strlen(key);
    if (key_len == 0) {
        return NULL;
    }

    len = strlen(plaintext);
    result = malloc(len + 1);
    if (result == NULL) {
        return NULL;
    }

    j = 0;
    for (i = 0; i < len; i++) {
        unsigned char c = (unsigned char)plaintext[i];

        if (isalpha(c)) {
            unsigned char kc = (unsigned char)key[j % key_len];
            int shift = 0;

            if (isalpha(kc)) {
                shift = toupper(kc) - 'A';
            }

            if (islower(c)) {
                result[i] = (char)('a' + (c - 'a' + shift) % 26);
            } else {
                result[i] = (char)('A' + (c - 'A' + shift) % 26);
            }

            j++;
        } else {
            result[i] = (char)c;
        }
    }

    result[len] = '\0';
    return result;
}

int main(int argc, char *argv[])
{
    char *encrypted;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <plaintext> <key>\n", argv[0]);
        return 1;
    }

    encrypted = encrypt_string(argv[1], argv[2]);
    if (encrypted == NULL) {
        fprintf(stderr, "Encryption failed\n");
        return 1;
    }

    printf("%s\n", encrypted);
    free(encrypted);

    return 0;
}