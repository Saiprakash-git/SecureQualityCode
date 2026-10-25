```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdint.h>

#define MAX_NAME_LEN   100
#define MAX_EMAIL_LEN  254
#define MIN_PASS_LEN   8
#define MAX_PASS_LEN   128
#define SALT_BYTES     16
#define SALT_HEX_LEN   (SALT_BYTES * 2)
#define HASH_BYTES     32
#define HASH_HEX_LEN   (HASH_BYTES * 2)
#define LINE_BUF_LEN   1024
#define DEFAULT_DB     "users.db"
#define HASH_ROUNDS    1000

typedef enum {
    REG_OK = 0,
    REG_ERR_NAME,
    REG_ERR_EMAIL,
    REG_ERR_PASSWORD,
    REG_ERR_DUPLICATE,
    REG_ERR_IO,
    REG_ERR_MEMORY
} RegStatus;

const char *reg_status_message(RegStatus status) {
    switch (status) {
        case REG_OK:            return "Registration successful.";
        case REG_ERR_NAME:      return "Invalid name (1-100 characters, no '|' or control characters).";
        case REG_ERR_EMAIL:     return "Invalid email address.";
        case REG_ERR_PASSWORD:  return "Invalid password (8-128 characters, no control characters).";
        case REG_ERR_DUPLICATE: return "An account with this email already exists.";
        case REG_ERR_IO:        return "Database I/O error.";
        case REG_ERR_MEMORY:    return "Out of memory.";
        default:                return "Unknown error.";
    }
}

int is_valid_name(const char *name) {
    size_t len, i;
    int has_visible = 0;

    if (name == NULL) return 0;
    len = strlen(name);
    if (len == 0 || len > MAX_NAME_LEN) return 0;

    for (i = 0; i < len; i++) {
        unsigned char c = (unsigned char)name[i];
        if (c == '|' || iscntrl(c)) return 0;
        if (!isspace(c)) has_visible = 1;
    }
    return has_visible;
}

int is_valid_email(const char *email) {
    size_t len, i, at_count = 0, at_pos = 0;

    if (email == NULL) return 0;
    len = strlen(email);
    if (len < 3 || len > MAX_EMAIL_LEN) return 0;

    for (i = 0; i < len; i++) {
        unsigned char c = (unsigned char)email[i];
        if (!isgraph(c) || c == '|') return 0;
        if (c == '@') {
            at_count++;
            at_pos = i;
        }
    }
    if (at_count != 1) return 0;
    if (at_pos == 0 || at_pos == len - 1) return 0;

    /* Domain must contain a dot that is not first or last character of domain */
    {
        const char *domain = email + at_pos + 1;
        const char *dot = strrchr(domain, '.');
        size_t dlen = strlen(domain);
        if (dot == NULL || dot == domain || dot == domain + dlen - 1) return 0;
    }
    return 1;
}

int is_valid_password(const char *password) {
    size_t len, i;

    if (password == NULL) return 0;
    len = strlen(password);
    if (len < MIN_PASS_LEN || len > MAX_PASS_LEN) return 0;

    for (i = 0; i < len; i++) {
        if (iscntrl((unsigned char)password[i])) return 0;
    }
    return 1;
}

static void normalize_email(char *dst, const char *src, size_t dst_size) {
    size_t i;
    for (i = 0; src[i] != '\0' && i + 1 < dst_size; i++) {
        dst[i] = (char)tolower((unsigned char)src[i]);
    }
    dst[i] = '\0';
}

static void bytes_to_hex(const unsigned char *in, size_t n, char *out) {
    static const char digits[] = "0123456789abcdef";
    size_t i;
    for (i = 0; i < n; i++) {
        out[2 * i]     = digits[in[i] >> 4];
        out[2 * i + 1] = digits[in[i] & 0x0F];
    }
    out[2 * n] = '\0';
}

static int hex_to_bytes(const char *hex, unsigned char *out, size_t n) {
    size_t i;
    for (i = 0; i < n; i++) {
        unsigned int v;
        if (sscanf(hex + 2 * i, "%2x", &v) != 1) return 0;
        out[i] = (unsigned char)v;
    }
    return 1;
}

void generate_salt(char *out_hex) {
    unsigned char salt[SALT_BYTES];
    size_t i;
    static int seeded = 0;

    if (!seeded) {
        srand((unsigned int)time(NULL) ^ (unsigned int)clock());
        seeded = 1;
    }
    for (i = 0; i < SALT_BYTES; i++) {
        salt[i] = (unsigned char)(rand() & 0xFF);
    }
    bytes_to_hex(salt, SALT_BYTES, out_hex);
}

/*
 * Salted iterated hash