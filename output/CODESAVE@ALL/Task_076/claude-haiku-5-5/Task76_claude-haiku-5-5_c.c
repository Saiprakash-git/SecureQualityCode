#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <curl/curl.h>

typedef struct {
    char *data;
    size_t len;
} Buffer;

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t total = size * nmemb;
    Buffer *buf = (Buffer *)userp;
    char *tmp = realloc(buf->data, buf->len + total + 1);
    if (!tmp) {
        return 0;
    }
    buf->data = tmp;
    memcpy(buf->data + buf->len, contents, total);
    buf->len += total;
    buf->data[buf->len] = '\0';
    return total;
}

char *fetch_url_content(const char *url) {
    CURL *curl;
    CURLcode res;
    Buffer buf = {NULL, 0};

    if (!url) {
        return NULL;
    }

    curl = curl_easy_init();
    if (!curl) {
        return NULL;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (title-extractor)");
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        free(buf.data);
        return NULL;
    }

    if (!buf.data) {
        buf.data = calloc(1, 1);
    }
    return buf.data;
}

static const char *find_ci(const char *haystack, const char *needle) {
    size_t nlen = strlen(needle);
    if (nlen == 0) {
        return haystack;
    }
    for (; *haystack; haystack++) {
        if (tolower((unsigned char)*haystack) == tolower((unsigned char)needle[0])) {
            size_t i;
            for (i = 1; i < nlen; i++) {
                if (!haystack[i] ||
                    tolower((unsigned char)haystack[i]) != tolower((unsigned char)needle[i])) {
                    break;
                }
            }
            if (i == nlen) {
                return haystack;
            }
        }
    }
    return NULL;
}

char *extract_page_title(const char *html) {
    const char *start_tag, *content_start, *end_tag;
    const char *s, *e;
    char *title;
    size_t len;

    if (!html) {
        return NULL;
    }

    start_tag = find_ci(html, "<title");
    if (!start_tag) {
        return NULL;
    }

    content_start = strchr(start_tag, '>');
    if (!content_start) {
        return NULL;
    }
    content_start++;

    end_tag = find_ci(content_start, "</title");
    if (!end_tag) {
        return NULL;
    }

    s = content_start;
    e = end_tag;
    while (s < e && isspace((unsigned char)*s)) {
        s++;
    }
    while (e > s && isspace((unsigned char)*(e - 1))) {
        e--;
    }

    len = (size_t)(e - s);
    title = malloc(len + 1);
    if (!title) {
        return NULL;
    }
    memcpy(title, s, len);
    title[len] = '\0';
    return title;
}

int main(int argc, char *argv[]) {
    int i;
    int status = 0;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <https-url> [more-urls...]\n", argv[0]);
        return 1;
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);

    for (i = 1; i < argc; i++) {
        char *html = fetch_url_content(argv[i]);
        char *title;

        if (!html) {
            fprintf(stderr, "Error: failed to retrieve %s\n", argv[i]);
            status = 1;
            continue;
        }

        title = extract_page_title(html);
        if (title) {
            printf("%s\n", title);
            free(title);
        } else {
            fprintf(stderr, "Error: no title found for %s\n", argv[i]);
            status = 1;
        }

        free(html);
    }

    curl_global_cleanup();
    return status;
}