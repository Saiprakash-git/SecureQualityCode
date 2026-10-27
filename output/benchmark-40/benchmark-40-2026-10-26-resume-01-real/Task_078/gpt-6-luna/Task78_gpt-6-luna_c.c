#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>

typedef struct {
    unsigned char *data;
    size_t len;
    size_t cap;
} Buffer;

static int buffer_append(Buffer *buffer, const void *data, size_t len)
{
    size_t needed;
    size_t capacity;
    unsigned char *new_data;

    if (len > SIZE_MAX - buffer->len)
        return 0;

    needed = buffer->len + len;
    if (needed > buffer->cap) {
        capacity = buffer->cap ? buffer->cap : 256;
        while (capacity < needed) {
            if (capacity > SIZE_MAX / 2) {
                capacity = needed;
                break;
            }
            capacity *= 2;
        }

        new_data = realloc(buffer->data, capacity);
        if (!new_data)
            return 0;

        buffer->data = new_data;
        buffer->cap = capacity;
    }

    if (len)
        memcpy(buffer->data + buffer->len, data, len);
    buffer->len = needed;
    return 1;
}

static int buffer_u32(Buffer *buffer, uint32_t value)
{
    unsigned char bytes[4];

    bytes[0] = (unsigned char)(value >> 24);
    bytes[1] = (unsigned char)(value >> 16);
    bytes[2] = (unsigned char)(value >> 8);
    bytes[3] = (unsigned char)value;
    return buffer_append(buffer, bytes, sizeof(bytes));
}

static int buffer_string(Buffer *buffer, const void *data, size_t len)
{
    return len <= UINT32_MAX &&
           buffer_u32(buffer, (uint32_t)len) &&
           buffer_append(buffer, data, len);
}

static int buffer_mpint(Buffer *buffer, const BIGNUM *number)
{
    unsigned char *bytes;
    int byte_count;
    int needs_zero;
    size_t encoded_len;
    int ok;

    if (!number || BN_is_negative(number))
        return 0;

    byte_count = BN_num_bytes(number);
    if (byte_count == 0)
        return buffer_u32(buffer, 0);

    needs_zero = BN_is_bit_set(number, BN_num_bits(number) - 1);
    encoded_len = (size_t)byte_count + (size_t)needs_zero;
    if (encoded_len > UINT32_MAX)
        return 0;

    bytes = malloc(encoded_len);
    if (!bytes)
        return 0;

    if (needs_zero)
        bytes[0] = 0;
    BN_bn2bin(number, bytes + needs_zero);

    ok = buffer_u32(buffer, (uint32_t)encoded_len) &&
         buffer_append(buffer, bytes, encoded_len);
    free(bytes);
    return ok;
}

static int make_public_blob(Buffer *public_blob,
                            const BIGNUM *exponent,
                            const BIGNUM *modulus)
{
    static const char key_type[] = "ssh-rsa";

    return buffer_string(public_blob, key_type, sizeof(key_type) - 1) &&
           buffer_mpint(public_blob, exponent) &&
           buffer_mpint(public_blob, modulus);
}

static int make_private_blob(Buffer *private_blob,
                             const BIGNUM *modulus,
                             const BIGNUM *exponent,
                             const BIGNUM *private_exponent,
                             const BIGNUM *iqmp,
                             const BIGNUM *prime1,
                             const BIGNUM *prime2,
                             const char *comment)
{
    static const char key_type[] = "ssh-rsa";
    unsigned char checkint[4];
    unsigned char padding = 1;
    size_t comment_len = strlen(comment);

    if (!RAND_bytes(checkint, sizeof(checkint)) ||
        !buffer_append(private_blob, checkint, sizeof(checkint)) ||
        !buffer_append(private_blob, checkint, sizeof(checkint)) ||
        !buffer_string(private_blob, key_type, sizeof(key_type) - 1) ||
        !buffer_mpint(private_blob, modulus) ||
        !buffer_mpint(private_blob, exponent) ||
        !buffer_mpint(private_blob, private_exponent) ||
        !buffer_mpint(private_blob, iqmp) ||
        !buffer_mpint(private_blob, prime1) ||
        !buffer_mpint(private_blob, prime2) ||
        !buffer_string(private_blob, comment, comment_len))
        return 0;

    while (private_blob->len % 8 != 0) {
        if (!buffer_append(private_blob, &padding, 1))
            return 0;
        ++padding;
    }

    return 1;
}

static int make_openssh_key(Buffer *openssh_key,
                            const Buffer *public_blob,
                            const Buffer *private_blob)
{
    static const char magic[] = "openssh-key-v1";
    static const char none[] = "none";
    static const char empty[] = "";

    return buffer_append(openssh_key, magic, sizeof(magic)) &&
           buffer_string(openssh_key, none, sizeof(none) - 1) &&
           buffer_string(openssh_key, none, sizeof(none) - 1) &&
           buffer_string(openssh_key, empty, 0) &&
           buffer_u32(openssh_key, 1) &&
           buffer_string(openssh_key, public_blob->data, public_blob->len) &&
           buffer_string(openssh_key, private_blob->data, private_blob->len);
}

static int write_pem(const Buffer *key)
{
    static const char header[] = "-----BEGIN OPENSSH PRIVATE KEY-----\n";
    static const char footer[] = "-----END OPENSSH PRIVATE KEY-----\n";
    unsigned char *encoded;
    size_t encoded_capacity;
    int encoded_len;
    size_t offset;
    int ok = 0;

    if (key->len > INT_MAX || key->len > (SIZE_MAX - 2) / 4 * 3)
        return 0;

    encoded_capacity = 4 * ((key->len + 2) / 3);
    if (encoded_capacity > INT_MAX)
        return 0;

    encoded = malloc(encoded_capacity + 1);
    if (!encoded)
        return 0;

    encoded_len = EVP_EncodeBlock(encoded, key->data, (int)key->len);
    if (encoded_len < 0 || (size_t)encoded_len != encoded_capacity)
        goto done;

    if (fwrite(header, 1, sizeof(header) - 1, stdout) != sizeof(header) - 1)
        goto done;

    for (offset = 0; offset < encoded_capacity; offset += 70) {
        size_t line_len = encoded_capacity - offset;
        if (line_len > 70)
            line_len = 70;
        if (fwrite(encoded + offset, 1, line_len, stdout) != line_len ||
            fputc('\n', stdout) == EOF)
            goto done;
    }

    if (fwrite(footer, 1, sizeof(footer) - 1, stdout) != sizeof(footer) - 1 ||
        fflush(stdout) != 0)
        goto done;

    ok = 1;

done:
    free(encoded);
    return ok;
}

static int parse_key_size(int argc, char **argv, int *bits)
{
    char *end;
    long value;

    *bits = 3072;
    if (argc < 2)
        return 1;
    if (argc > 3)
        return 0;

    errno = 0;
    value = strtol(argv[1], &end, 10);
    if (errno || end == argv[1] || *end != '\0' ||
        value < 2048 || value > 16384)
        return 0;

    *bits = (int)value;
    return 1;
}

int main(int argc, char **argv)
{
    EVP_PKEY_CTX *context = NULL;
    EVP_PKEY *key = NULL;
    RSA *rsa = NULL;
    const BIGNUM *modulus = NULL;
    const BIGNUM *exponent = NULL;
    const BIGNUM *private_exponent = NULL;
    const BIGNUM *prime1 = NULL;
    const BIGNUM *prime2 = NULL;
    const BIGNUM *exponent1 = NULL;
    const BIGNUM *exponent2 = NULL;
    const BIGNUM *iqmp = NULL;
    Buffer public_blob = {0};
    Buffer private_blob = {0};
    Buffer openssh_key = {0};
    const char *comment = argc == 3 ? argv[2] : "generated";
    int bits;
    int status = EXIT_FAILURE;

    if (!parse_key_size(argc, argv, &bits)) {
        fprintf(stderr, "Usage: %s [bits [comment]]\n", argv[0]);
        goto cleanup;
    }

    context = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
    if (!context ||
        EVP_PKEY_keygen_init(context) <= 0 ||
        EVP_PKEY_CTX_set_rsa_keygen_bits(context, bits) <= 0 ||
        EVP_PKEY_keygen(context, &key) <= 0) {
        fprintf(stderr, "RSA key generation failed\n");
        goto cleanup;
    }

    rsa = EVP_PKEY_get1_RSA(key);
    if (!rsa) {
        fprintf(stderr, "Unable to access generated RSA key\n");
        goto cleanup;
    }

    RSA_get0_key(rsa, &modulus, &exponent, &private_exponent);
    RSA_get0_factors(rsa, &prime1, &prime2);
    RSA_get0_crt_params(rsa, &exponent1, &exponent2, &iqmp);
    (void)exponent1;
    (void)exponent2;

    if (!make_public_blob(&public_blob, exponent, modulus) ||
        !make_private_blob(&private_blob, modulus, exponent,
                           private_exponent, iqmp, prime1, prime2, comment) ||
        !make_openssh_key(&openssh_key, &public_blob, &private_blob)) {
        fprintf(stderr, "Unable to encode OpenSSH private key\n");
        goto cleanup;
    }

    if (!write_pem(&openssh_key)) {
        fprintf(stderr, "Unable to write OpenSSH private key\n");
        goto cleanup;
    }

    status = EXIT_SUCCESS;

cleanup:
    free(public_blob.data);
    free(private_blob.data);
    free(openssh_key.data);
    RSA_free(rsa);
    EVP_PKEY_free(key);
    EVP_PKEY_CTX_free(context);
    return status;
}