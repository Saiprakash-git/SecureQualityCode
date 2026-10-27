#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <openssl/rsa.h>
#include <openssl/bn.h>
#include <openssl/rand.h>

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

typedef struct {
    uint8_t *data;
    size_t len;
    size_t cap;
} Buffer;

static void buf_init(Buffer *b) {
    b->data = NULL;
    b->len = 0;
    b->cap = 0;