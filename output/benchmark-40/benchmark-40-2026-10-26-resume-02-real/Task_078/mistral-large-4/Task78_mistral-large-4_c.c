#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/encoder.h>

char *generate_rsa_openssh_private_key(int bits) {
    EVP_PKEY_CTX *pctx = NULL;
    EVP_PKEY *pkey = NULL;
    BIO *bio = NULL;
    OSSL_ENCODER_CTX *ectx = NULL;
    char *data = NULL;
    char *result = NULL;
    long len = 0;

    pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
    if (!pctx) {
        ERR_print_errors_fp(stderr);
        goto cleanup;
    }

    if (EVP_PKEY_keygen_init(pctx) <= 0) {
        ERR_print_errors_fp(stderr);
        goto cleanup;
    }

    if (EVP_PKEY_CTX_set_rsa_keygen_bits(pctx, bits) <= 0) {
        ERR_print_errors_fp(stderr);
        goto cleanup;
    }

    if (EVP_PKEY_keygen(pctx, &pkey) <= 0) {
        ERR_print_errors_fp(stderr);
        goto cleanup;
    }

    bio = BIO_new(BIO_s_mem());
    if (!bio) {
        ERR_print_errors_fp(stderr);
        goto cleanup;
    }

    ectx = OSSL_ENCODER_CTX_new_for_pkey(pkey,
        OSSL_KEYMGMT_SELECT_PRIVATE_KEY,
        "OpenSSH", NULL, NULL);
    if (!ectx) {
        ERR_print_errors_fp(stderr);
        goto cleanup;
    }

    if (OSSL_ENCODER_to_bio(ectx, bio) <= 0) {
        ERR_print_errors_fp(stderr);
        goto cleanup;
    }

    len = BIO_get_mem_data(bio, &data);
    if (len <= 0) {
        goto cleanup;
    }

    result = malloc(len + 1);
    if (!result) {
        goto cleanup;
    }

    memcpy(result, data, len);
    result[len] = '\0';

cleanup:
    OSSL_ENCODER_CTX_free(ectx);
    BIO_free(bio);
    EVP_PKEY_free(pkey);
    EVP_PKEY_CTX_free(pctx);
    return result;
}

int main(void) {
    char *key = generate_rsa_openssh_private_key(2048);
    if (key) {
        printf("%s", key);
        free(key);
        return 0;
    }
    return 1;
}