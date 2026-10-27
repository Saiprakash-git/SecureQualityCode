#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#define MAX_HEADERS 32
#define MAX_HEADER_LEN 512
#define MAX_BODY_LEN 4096
#define MAX_RESPONSE_LEN 8192

typedef struct {
    char name[MAX_HEADER_LEN];
    char value[MAX_HEADER_LEN];
} HttpHeader;

typedef struct {
    char method[16];
    char path[256];
    HttpHeader headers[MAX_HEADERS];
    size_t header_count;
    char body[MAX_BODY_LEN];
} HttpRequest;

typedef struct {
    int status_code;
    char status_text[64];
    HttpHeader headers[MAX_HEADERS];
    size_t header_count;
    char body[MAX_BODY_LEN];
} HttpResponse;

typedef struct {
    const char **allowed_origins;
    size_t allowed_origins_count;
    bool allow_all_origins;
    const char *allowed_methods;
    const char *allowed_headers;
    const char *exposed_headers;
    bool allow_credentials;
    int max_age_seconds;
} CorsConfig;

static int strcasecmp_custom(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        int diff = tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
        if (diff != 0) {
            return diff;
        }
        s1++;
        s2++;
    }
    return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}

static void trim_whitespace(char *str) {
    char *end;
    while (isspace((unsigned char)*str)) {
        str++;
    }
    if (*str == 0) {
        return;
    }
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) {
        end--;
    }
    end[1] = '\0';
}

static const char *get_header(const HttpRequest *req, const char *name) {
    for (size_t i = 0; i < req->header_count; i++) {
        if (strcasecmp_custom(req->headers[i].name, name) == 0) {
            return req->headers[i].value;
        }
    }
    return NULL;
}

static bool add_response_header(HttpResponse *res, const char *name, const char *value) {
    if (res->header_count >= MAX_HEADERS) {
        return false;
    }
    snprintf(res->headers[res->header_count].name, MAX_HEADER_LEN, "%s", name);
    snprintf(res->headers[res->header_count].value, MAX_HEADER_LEN, "%s", value);
    res->header_count++;
    return true;
}

static void set_response_status(HttpResponse *res, int code, const char *text) {
    res->status_code = code;
    snprintf(res->status_text, sizeof(res->status_text), "%s", text);
}

static void set_response_body(HttpResponse *res, const char *body, const char *content_type) {
    snprintf(res->body, sizeof(res->body), "%s", body);
    add_response_header(res, "Content-Type", content_type);
    char len_buf[32];
    snprintf(len_buf, sizeof(len_buf), "%zu", strlen(body));
    add_response_header(res, "Content-Length", len_buf);
}

static bool is_origin_allowed(const CorsConfig *config, const char *origin) {
    if (!origin) {
        return false;
    }
    if (config->allow_all_origins) {
        return true;
    }
    for (size_t i = 0; i < config->allowed_origins_count; i++) {
        if (strcmp(config->allowed_origins[i], origin) == 0) {
            return true;
        }
    }
    return false;
}

static bool is_token_in_csv(const char *csv_list, const char *token) {
    if (!csv_list || !token) {
        return false;
    }
    char buffer[MAX_HEADER_LEN];
    snprintf(buffer, sizeof(buffer), "%s", csv_list);
    char *saveptr = NULL;
    char *item = strtok_r(buffer, ",", &saveptr);
    while (item) {
        while (isspace((unsigned char)*item)) item++;
        char *end = item + strlen(item) - 1;
        while (end > item && isspace((unsigned char)*end)) {
            *end = '\0';
            end--;
        }
        if (strcasecmp_custom(item, token) == 0) {
            return true;
        }
        item = strtok_r(NULL, ",", &saveptr);
    }
    return false;
}

static bool are_headers_allowed(const CorsConfig *config, const char *requested_headers) {
    if (!requested_headers || strlen(requested_headers) == 0) {
        return true;
    }
    if (!config->allowed_headers) {
        return false;
    }
    if (strcmp(config->allowed_headers, "*") == 0) {
        return true;
    }
    char buffer[MAX_HEADER_LEN];
    snprintf(buffer, sizeof(buffer), "%s", requested_headers);
    char *saveptr = NULL;
    char *header_token = strtok_r(buffer, ",", &saveptr);
    while (header_token) {
        while (isspace((unsigned char)*header_token)) header_token++;
        char *end = header_token + strlen(header_token) - 1;
        while (end > header_token && isspace((unsigned char)*end)) {
            *end = '\0';
            end--;
        }
        if (!is_token_in_csv(config->allowed_headers, header_token)) {
            return false;
        }
        header_token = strtok_r(NULL, ",", &saveptr);
    }
    return true;
}

static bool is_cors_request(const HttpRequest *req) {
    return get_header(req, "Origin") != NULL;
}

static bool is_preflight_request(const HttpRequest *req) {
    return (strcmp(req->method, "OPTIONS") == 0) &&
           (get_header(req, "Origin") != NULL) &&
           (get_header(req, "Access-Control-Request-Method") != NULL);
}

static void apply_cors_preflight(const CorsConfig *config, const HttpRequest *req, HttpResponse *res) {
    const char *origin = get_header(req, "Origin");
    const char *req_method = get_header(req, "Access-Control-Request-Method");
    const char *req_headers = get_header(req, "Access-Control-Request-Headers");

    if (!is_origin_allowed(config, origin)) {
        set_response_status(res, 403, "Forbidden");
        set_response_body(res, "{\"error\": \"CORS origin denied\"}", "application/json");
        return;
    }

    if (!is_token_in_csv(config->allowed_methods, req_method)) {
        set_response_status(res, 405, "Method Not Allowed");
        set_response_body(res, "{\"error\": \"CORS method not allowed\"}", "application/json");
        return;
    }

    if (!are_headers_allowed(config, req_headers)) {
        set_response_status(res, 403, "Forbidden");
        set_response_body(res, "{\"error\": \"CORS header not allowed\"}", "application/json");
        return;
    }

    set_response_status(res, 204, "No Content");

    if (config->allow_credentials) {
        add_response_header(res, "Access-Control-Allow-Origin", origin);
        add_response_header(res, "Access-Control-Allow-Credentials", "true");
    } else {
        if (config->allow_all_origins) {
            add_response_header(res, "Access-Control-Allow-Origin", "*");
        } else {
            add_response_header(res, "Access-Control-Allow-Origin", origin);
        }
    }

    add_response_header(res, "Access-Control-Allow-Methods", config->allowed_methods);
    if (config->allowed_headers) {
        add_response_header(res, "Access-Control-Allow-Headers", config->allowed_