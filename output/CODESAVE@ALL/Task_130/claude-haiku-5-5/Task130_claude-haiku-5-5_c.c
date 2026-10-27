#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct {
    const char *const *origins;
    size_t origin_count;
    int allow_any_origin;

    const char *const *methods;
    size_t method_count;

    const char *const *headers;
    size_t header_count;

    const char *const *exposed_headers;
    size_t exposed_count;

    int allow_credentials;
    long max_age;
} cors_policy;

typedef struct {
    const char *method;
    const char *origin;
    const char *request_method;
    const char *request_headers;
} cors_request;

typedef enum {
    CORS_NOT_CORS,
    CORS_SIMPLE_ALLOWED,
    CORS_PREFLIGHT_ALLOWED,
    CORS_REJECTED
} cors_status;

typedef struct {
    cors_status status;
    int is_preflight;
    int allow_credentials;
    char allow_origin[512];
    char allow_methods[256];
    char allow_headers[512];
    char expose_headers[512];
    char max_age[32];
    int vary_origin;
} cors_result;

static int ieq(const char *a, const char *b) {
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        a++;
        b++;
    }
    return *a == '\0' && *b == '\0';
}

static int in_list(const char *value, const char *const *list, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (ieq(value, list[i])) return 1;
    }
    return 0;
}

static int headers_allowed(const char *csv, const char *const *allowed, size_t n) {
    static const char *const safelisted[] = { "accept", "accept-language", "content-language" };
    if (!csv) return 1;

    const char *p = csv;
    while (*p) {
        while (*p == ',' || isspace((unsigned char)*p)) p++;
        if (!*p) break;

        const char *start = p;
        while (*p && *p != ',' && !isspace((unsigned char)*p)) p++;

        size_t tl = (size_t)(p - start);
        char tok[128];
        if (tl == 0 || tl >= sizeof tok) return 0;
        memcpy(tok, start, tl);
        tok[tl] = '\0';

        if (!in_list(tok, allowed, n) && !in_list(tok, safelisted, sizeof safelisted / sizeof safelisted[0]))
            return 0;
    }
    return 1;
}

static void buf_append(char *dst, size_t cap, size_t *len, const char *s) {
    while (*s && *len + 1 < cap) {
        dst[(*len)++] = *s++;
    }
    dst[*len] = '\0';
}

static void join_list(char *dst, size_t cap, const char *const *list, size_t n) {
    size_t len = 0;
    if (cap == 0) return;
    dst[0] = '\0