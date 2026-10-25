#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define REQ_BUF_SIZE   16384
#define FIELD_MAX      1024
#define PAGE_BUF_SIZE  65536

static int hex_value(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static void url_decode(const char *src, char *dst, size_t dstlen)
{
    size_t di = 0;
    if (dstlen == 0) return;
    for (size_t si = 0; src[si] && di + 1 < dstlen; si++) {
        if (src[si] == '+') {
            dst[di++] = ' ';
        } else if (src[si] == '%' && src[si + 1] && src[si + 2]
                   && hex_value(src[si + 1]) >= 0 && hex_value(src[si + 2]) >= 0) {
            dst[di++] = (char)(hex_value(src[si + 1]) * 16 + hex_value(src[si + 2]));
            si += 2;
        } else {
            dst[di++] = src[si];
        }
    }
    dst[di] = '\0';
}

static void html_escape(const char *src, char *dst, size_t dstlen)
{
    size_t di = 0;
    if (dstlen == 0) return;
    for (size_t si = 0; src[si]; si++) {
        const char *rep = NULL;
        switch (src[si]) {
            case '&':  rep = "&amp;";  break;
            case '<':  rep = "&lt;";   break;
            case '>':  rep = "&gt;";   break;
            case '"':  rep = "&quot;"; break;
            case '\'': rep = "&#39;";  break;
            default:   break;
        }
        if (rep) {
            size_t rl = strlen(rep);
            if (di + rl >= dstlen) break;
            memcpy(dst + di, rep, rl);
            di += rl;
        } else {
            if (di + 1 >= dstlen) break;
            dst[di++] = src[si];
        }
    }
    dst[di] = '\0';
}

static int get_form_value(const char *body, const char *key, char *out, size_t outlen)
{
    size_t keylen = strlen(key);
    const char *p = body;
    char raw[FIELD_MAX * 2];

    out[0] = '\0';
    while (*p) {
        const char *amp = strchr(p, '&');
        size_t pairlen = amp ? (size_t)(amp - p) : strlen(p);
        const char *eq = memchr(p, '=', pairlen);
        size_t namelen = eq ? (size_t)(eq - p) : pairlen;

        if (namelen == keylen && strncmp(p, key, keylen) == 0) {
            size_t vlen = eq ? pairlen - namelen - 1 : 0;
            if (vlen >= sizeof(raw)) vlen = sizeof(raw) - 1;
            if (eq) {
                memcpy(raw, eq + 1, vlen);
                raw[vlen] = '\0';
                url_decode(raw, out, outlen);
            }
            return 1;
        }
        if (!amp) break;
        p = amp + 1;
    }
    return 0;
}

static size_t build_form_page(char *buf, size_t buflen)
{
    return (size_t)snprintf(buf, buflen,
        "<!DOCTYPE html>\n"
        "<html><head><meta charset=\"utf-8\"><title>Input Form</title>\n"
        "<style>body{font-family:sans-serif;max-width:600px;margin:40px auto;}"
        "label{display:block;margin-top:12px;font-weight:bold;}"
        "input,textarea{width:100%%;padding:6px;box-sizing:border-box;}"
        "button{margin-top:16px;padding:8px 16px;}</style>\n"
        "</head><body>\n"
        "<h1>Submit Your Information</h1>\n"
        "<form method=\"POST\" action=\"/submit\">\n"
        "<label for=\"name\">Name</label>\n"
        "<input type=\"text\" id=\"name\" name=\"name\" required>\n"
        "<label for=\"email\">Email</label>\n"
        "<input type=\"email\" id=\"email\" name=\"email\">\n"
        "<label for=\"message\">Message</label>\n"
        "<textarea id=\"message\" name=\"message\" rows=\"5\"></textarea>\n"
        "<button type=\"submit\">Submit</button>\n"
        "</form>\n"
        "</body></html>\n");
}

static size_t build_result_page(char *buf, size_t buflen,
                                const char *name, const char *email,
                                const char *message)
{
    char e_name[FIELD_MAX * 6];
    char e_email[FIELD_MAX * 6];
    char e_message[FIELD_MAX * 6];

    html_escape(name, e_name, sizeof(e_name));
    html_escape(email, e_email, sizeof(e_email));
    html_escape(message, e_message, sizeof(e_message));

    return (size_t)snprintf(buf, buflen,
        "<!DOCTYPE html>\n"
        "<html><head><meta charset=\"utf-8\"><title>Your Submission</title>\n"
        "<style>body{font-family:sans-serif;max-width:600px;margin:40px auto;}"
        "dt{font-weight:bold;margin-top:12px;}dd{margin-left:0;white-space:pre-wrap;}"
        "a{display:inline-block;margin-top:20px;}</style>\n"
        "</head><body>\n"
        "<h1>Thanks for your submission</h1>\n"
        "<dl>\n"
        "<dt>Name</dt><dd>%s</dd>\n"
        "<dt>Email</dt><dd>%s</dd>\n"
        "<dt>Message</dt><dd>%s</dd>\n"
        "</dl>\n"
        "<a href=\"/\">Submit another</a>\n"
        "</body></html>\n",
        e_name[0] ? e_name : "(not provided)",
        e_email[0] ? e_email : "(not provided)",
        e_message[0] ? e_message : "(not provided)");
}

static size_t build_not_found_page(char *buf, size_t buflen)
{
    return (size_t)snprintf(buf, buflen,
        "<!DOCTYPE html>\n<html><head><meta charset=\"utf-8\"><title>404</title></head>"
        "<body><h1>404 Not Found</h1><p><a href=\"/\">Home</a></p></body></html>\n");
}

static void send_all(int fd, const char *data, size_t len)
{
    size_t sent = 0;
    while (sent < len) {
        ssize_t w = send(fd, data + sent, len - sent, 0);
        if (w <= 0) return;
        sent += (size_t)w;
    }
}

static void send_response(int fd, const char *status, const char *body, size_t body_len)
{
    char header[512];
    int hl = snprintf(header, sizeof(header),
        "HTTP/1.1 %s\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n\r\n",
        status, body_len);
    send_all(fd, header, (size_t)hl);
    send_all(fd, body, body_len);
}

static long parse_content_length(const char *headers)
{
    const char *line = headers;
    while (line && *line) {
        if (strncasecmp(line, "Content-Length:", 15) == 0)
            return atol(line + 15);
        line = strstr(line, "\r\n");
        if (line) line += 2;
    }
    return 0;
}

static void handle_client(int fd)
{
    static char req[REQ_BUF_SIZE];
    size_t total = 0;