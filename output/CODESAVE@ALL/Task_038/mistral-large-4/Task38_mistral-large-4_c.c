#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_INPUT 10240
#define MAX_FIELD 4096

char *url_decode(const char *src) {
    char *dst = malloc(strlen(src) + 1);
    char *d = dst;
    const char *s = src;
    while (*s) {
        if (*s == '%' && s[1] && s[2]) {
            int hex;
            sscanf(s + 1, "%2x", &hex);
            *d++ = (char)hex;
            s += 3;
        } else if (*s == '+') {
            *d++ = ' ';
            s++;
        } else {
            *d++ = *s++;
        }
    }
    *d = '\0';
    return dst;
}

void html_escape(const char *src, char *dst, size_t dst_size) {
    size_t j = 0;
    for (size_t i = 0; src[i] && j < dst_size - 6; i++) {
        switch (src[i]) {
            case '<': memcpy(dst + j, "&lt;", 4); j += 4; break;
            case '>': memcpy(dst + j, "&gt;", 4); j += 4; break;
            case '&': memcpy(dst + j, "&amp;", 5); j += 5; break;
            case '"': memcpy(dst + j, "&quot;", 6); j += 6; break;
            case '\'': memcpy(dst + j, "&#39;", 5); j += 5; break;
            default: dst[j++] = src[i]; break;
        }
    }
    dst[j] = '\0';
}

void render_page(const char *name, const char *message, int submitted) {
    char safe_name[MAX_FIELD * 2];
    char safe_message[MAX_FIELD * 2];

    html_escape(name ? name : "", safe_name, sizeof(safe_name));
    html_escape(message ? message : "", safe_message, sizeof(safe_message));

    printf("Content-Type: text/html\r\n\r\n");
    printf("<!DOCTYPE html>\n<html>\n<head>\n");
    printf("<meta charset='UTF-8'>\n");
    printf("<title>Form Echo</title>\n");
    printf("<