#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <openssl/rsa.h>
#include <openssl/bn.h>
#include <openssl/rand.h>

typedef struct {
    unsigned char *d;
    size_t len;
    size_t cap;
} buf_t;

static void die(const char *msg)
{
    fprintf(stderr, "error: %s\n", msg);
    exit(1);
}

static void buf_reserve(buf_t *b, size_t extra)
{
    if (b->len + extra > b->cap) {
        size_t nc = b->cap ? b->cap : 256;
        while (nc < b->len + extra)
            nc *= 2;
        unsigned char *nd = realloc(b->d, nc);
        if (!nd)
            die("out of memory");
        b->d = nd;
        b->cap = nc;
    }
}

static void buf_append(buf_t *b, const void *p, size_t n)
{
    buf_reserve(b, n);
    if (n)
        memcpy(b->d + b->len, p, n);
    b->len += n;
}

static void buf_u32(buf_t *b, uint32_t v)
{
    unsigned char t[4];
    t[0] = (unsigned char)(v >> 24);
    t[1] = (unsigned char)(v >> 16);
    t[2] = (unsigned char)(v >> 8);
    t[3] = (unsigned char)(v);
    buf_append(b, t, 4);
}

static void buf_str(buf_t *b, const void *p, size_t n)
{
    buf_u32(b, (uint32_t)n);
    buf_append(b, p, n);
}

static void buf_mpint(buf_t *b, const BIGNUM *bn)
{
    int len = BN_num_bytes(bn);
    if (len == 0) {
        buf_u32(b, 0);
        return;
    }
    unsigned char *tmp = malloc((size_t)len + 1);
    if (!tmp)
        die("out of memory");
    BN_bn2bin(bn, tmp + 1);
    if (tmp[1] & 0x80) {
        tmp[0] = 0;
        buf_str(b, tmp, (size_t)len + 1);
    } else {
        buf_str(b, tmp + 1, (size_t)len);
    }
    free(tmp);
}

static char *base64_encode(const unsigned char *in, size_t n)
{
    static const char tbl[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t olen = ((n + 2) / 3) * 4;
    char *out = malloc(olen + 1);
    if (!out)
        die("out of memory");
    size_t i, j = 0;
    for (i = 0; i + 2 < n; i += 3) {
        uint32_t v = ((uint32_t)in[i] << 16) | ((uint32_t)in[i + 1] << 8) | in[i + 2];
        out[j++] = tbl[(v >> 18) & 63];
        out[j++] = tbl[(v >> 12) & 63];
        out[j++] = tbl[(v >> 6) & 63];
        out[j++] = tbl[v & 63];
    }
    if (n - i == 1) {
        uint32_t v = (uint32_t)in[i] << 16;
        out[j++] = tbl[(v >> 18) & 63];
        out[j++] = tbl[(v >> 12) & 63];
        out[j++] = '=';
        out[j++] = '=';
    } else if (n - i == 2) {
        uint32_t v = ((uint32_t)in[i] << 16) | ((uint32_t)in[i + 1] << 8);
        out[j++] = tbl[(v >> 18) & 63];
        out[j++] = tbl[(v >> 12) & 63];
        out[j++] = tbl[(v >> 6) & 63];
        out[j++] = '=';
    }
    out[j] = '\0';
    return out;
}

static RSA *generate_rsa_key(int bits)
{
    RSA *rsa = RSA_new();
    BIGNUM *e = BN_new();
    if (!rsa || !e)
        die("allocation failed");
    if (!BN_set_word(e, 65537))
        die("failed to set exponent");
    if (!RSA_generate_key_ex(rsa, bits, e, NULL))
        die("RSA key generation failed");
    BN_free(e);
    return rsa;
}

static void export_openssh_private_key(RSA *rsa, const char *comment, buf_t *out)
{
    const BIGNUM *n, *e, *d, *p, *q, *dmp1, *dmq1, *iqmp;
    RSA_get0_key(rsa, &n, &e, &d);
    RSA_get0_factors(rsa, &p, &q);
    RSA_get0_crt_params(rsa,