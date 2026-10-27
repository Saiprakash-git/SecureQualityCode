#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

#define SALT_SIZE 16
#define NONCE_SIZE 12
#define TAG_SIZE 16
#define HEADER_SIZE 36
#define MAX_SECRET_SIZE (1024U * 1024U)
#define MAX_PASSWORD_SIZE 4096U
#define PBKDF2_ITERATIONS 600000

static const unsigned char file_magic[8] = {'S', 'V', 'A', 'U', 'L', 'T', '0', '1'};

static int write_all(int fd, const unsigned char *data, size_t length)
{
    size_t offset = 0;

    while (offset < length) {
        ssize_t written = write(fd, data + offset, length - offset);
        if (written < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (written == 0)
            return -1;
        offset += (size_t)written;
    }
    return 0;
}

static int read_line_from_tty(int fd, const char *prompt, unsigned char *buffer,
                              size_t capacity, size_t *length, int hide_input)
{
    struct termios original;
    struct termios hidden;
    int have_termios = 0;
    size_t used = 0;
    int overflow = 0;
    int result = -1;

    if (capacity == 0 || write_all(fd, (const unsigned char *)prompt, strlen(prompt)) != 0)
        return -1;

    if (hide_input) {
        if (tcgetattr(fd, &original) != 0)
            goto done;
        hidden = original;
        hidden.c_lflag &= (tcflag_t)~ECHO;
        if (tcsetattr(fd, TCSAFLUSH, &hidden) != 0)
            goto done;
        have_termios = 1;
    }

    for (;;) {
        unsigned char ch;
        ssize_t n = read(fd, &ch, 1);

        if (n < 0) {
            if (errno == EINTR)
                continue;
            goto done;
        }
        if (n == 0)
            goto done;
        if (ch == '\n' || ch == '\r')
            break;

        if (used + 1 < capacity)
            buffer[used++] = ch;
        else
            overflow = 1;
    }

    if (hide_input && write_all(fd, (const unsigned char *)"\n", 1) != 0)
        goto done;

    if (overflow)
        goto done;

    buffer[used] = '\0';
    *length = used;
    result = 0;

done:
    if (have_termios && tcsetattr(fd, TCSAFLUSH, &original) != 0)
        result = -1;
    if (result != 0)
        OPENSSL_cleanse(buffer, capacity);
    return result;
}

static int derive_key(const unsigned char *password, size_t password_length,
                      const unsigned char salt[SALT_SIZE],
                      unsigned char key[32])
{
    if (password_length > INT_MAX)
        return -1;

    return PKCS5_PBKDF2_HMAC((const char *)password, (int)password_length,
                             salt, SALT_SIZE, PBKDF2_ITERATIONS,
                             EVP_sha256(), 32, key) == 1 ? 0 : -1;
}

static int encrypt_secret(const unsigned char *secret, size_t secret_length,
                          const unsigned char *password, size_t password_length,
                          unsigned char **result, size_t *result_length)
{
    unsigned char key[32];
    unsigned char *output = NULL;
    EVP_CIPHER_CTX *ctx = NULL;
    int out_length = 0;
    int final_length = 0;
    size_t total_length;
    int ok = 0;

    if (secret_length > MAX_SECRET_SIZE || secret_length > INT_MAX)
        return -1;

    total_length = HEADER_SIZE + secret_length + TAG_SIZE;
    output = malloc(total_length);
    if (output == NULL)
        goto done;

    memcpy(output, file_magic, sizeof(file_magic));
    if (RAND_bytes(output + 8, SALT_SIZE) != 1 ||
        RAND_bytes(output + 8 + SALT_SIZE, NONCE_SIZE) != 1 ||
        derive_key(password, password_length, output + 8, key) != 0)
        goto done;

    ctx = EVP_CIPHER_CTX_new();
    if (ctx == NULL ||
        EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL) != 1 ||
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, NONCE_SIZE, NULL) != 1 ||
        EVP_EncryptInit_ex(ctx, NULL, NULL, key, output + 8 + SALT_SIZE) != 1 ||
        EVP_EncryptUpdate(ctx, NULL, &out_length, output, HEADER_SIZE) != 1)
        goto done;

    if (secret_length != 0 &&
        EVP_EncryptUpdate(ctx, output + HEADER_SIZE, &out_length, secret,
                          (int)secret_length) != 1)
        goto done;
    if (secret_length == 0)
        out_length = 0;

    if (EVP_EncryptFinal_ex(ctx, output + HEADER_SIZE + out_length,
                            &final_length) != 1)
        goto done;
    if ((size_t)(out_length + final_length) != secret_length)
        goto done;

    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, TAG_SIZE,
                            output + HEADER_SIZE + secret_length) != 1)
        goto done;

    *result = output;
    *result_length = total_length;
    output = NULL;
    ok = 1;

done:
    OPENSSL_cleanse(key, sizeof(key));
    if (ctx != NULL)
        EVP_CIPHER_CTX_free(ctx);
    if (output != NULL) {
        OPENSSL_cleanse(output, total_length);
        free(output);
    }
    return ok ? 0 : -1;
}

static int decrypt_secret(const unsigned char *file_data, size_t file_length,
                          const unsigned char *password, size_t password_length,
                          unsigned char **result, size_t *result_length)
{
    unsigned char key[32];
    unsigned char *plaintext = NULL;
    EVP_CIPHER_CTX *ctx = NULL;
    size_t ciphertext_length;
    int out_length = 0;
    int final_length = 0;
    int ok = 0;

    if (file_length < HEADER_SIZE + TAG_SIZE ||
        memcmp(file_data, file_magic, sizeof(file_magic)) != 0)
        return -1;

    ciphertext_length = file_length - HEADER_SIZE - TAG_SIZE;
    if (ciphertext_length > MAX_SECRET_SIZE || ciphertext_length > INT_MAX)
        return -1;

    plaintext = malloc(ciphertext_length + 1);
    if (plaintext == NULL)
        goto done;

    if (derive_key(password, password_length, file_data + 8, key) != 0)
        goto done;

    ctx = EVP_CIPHER_CTX_new();
    if (ctx == NULL ||
        EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL) != 1 ||
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, NONCE_SIZE, NULL) != 1 ||
        EVP_DecryptInit_ex(ctx, NULL, NULL, key, file_data + 8 + SALT_SIZE) != 1 ||
        EVP_DecryptUpdate(ctx, NULL, &out_length, file_data, HEADER_SIZE) != 1)
        goto done;

    if (ciphertext_length != 0 &&
        EVP_DecryptUpdate(ctx, plaintext, &out_length,
                          file_data + HEADER_SIZE, (int)ciphertext_length) != 1)
        goto done;
    if (ciphertext_length == 0)
        out_length = 0;

    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, TAG_SIZE,
                            (void *)(file_data + HEADER_SIZE + ciphertext_length)) != 1)
        goto done;
    if (EVP_DecryptFinal_ex(ctx, plaintext + out_length, &final_length) != 1)
        goto done;
    if ((size_t)(out_length + final_length) != ciphertext_length)
        goto done;

    plaintext[ciphertext_length] = '\0';
    *result = plaintext;
    *result_length = ciphertext_length;
    plaintext = NULL;
    ok = 1;

done:
    OPENSSL_cleanse(key, sizeof(key));
    if (ctx != NULL)
        EVP_CIPHER_CTX_free(ctx);
    if (plaintext != NULL) {
        OPENSSL_cleanse(plaintext, ciphertext_length + 1);
        free(plaintext);
    }
    return ok ? 0 : -1;
}

static int read_encrypted_file(const char *path, unsigned char **data, size_t *length)
{
    int fd = -1;
    struct stat st;
    unsigned char *buffer = NULL;
    size_t offset = 0;
    int result = -1;

    fd = open(path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0)
        goto done;
    if (fstat(fd, &st) != 0 || !S_ISREG(st.st_mode) || st.st_size < 0 ||
        (uintmax_t)st.st_size > HEADER_SIZE + MAX_SECRET_SIZE + TAG_SIZE)
        goto done;

    *length = (size_t)st.st_size;
    buffer = malloc(*length == 0 ? 1 : *length);
    if (buffer == NULL)
        goto done;

    while (offset < *length) {
        ssize_t n = read(fd, buffer + offset, *length - offset);
        if (n <