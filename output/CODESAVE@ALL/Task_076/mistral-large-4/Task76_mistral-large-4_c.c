#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <curl/curl.h>

struct Buffer {
    char *data;
    size_t size;
};

static size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t total = size * nmemb;
    struct Buffer *buf = (struct Buffer *)userdata;
    char *new_data = realloc(buf->data, buf->size + total + 1);
    if (!new_data) return 0;
    buf->data = new_data;
    memcpy(buf->data + buf->size, ptr, total);
    buf->size += total;
    buf->data[buf->size] = '\0';
    return total;
}

static const char *find_case_insensitive(const char *haystack, const char *needle) {
    if (!*needle) return haystack;
    for (; *haystack; haystack++) {
        const char *h = haystack;
        const char *n = needle;
        while (*h && *n && tolower((unsigned char)*h) == tolower((unsigned char)*n)) {
            h++;
            n++;
        }
        if (!*n) return haystack;
    }
    return NULL;
}

static char *extract_title(const char *html) {
    const char *start = find_case_insensitive(html, "<title>");
    if (!start) return NULL;
    start += 7;
    const char *end = find_case_insensitive(start, "</title>");
    if (!end) return NULL;
    size_t len = end - start;
    char *title = malloc(len + 1);
    if (!title) return NULL;
    memcpy(title, start, len);
    title[len] = '\0';
    char *begin = title;
    while (*begin && isspace((unsigned char)*begin)) begin++;
    char *finish = begin + strlen(begin);
    while (finish > begin && isspace((unsigned char)*(finish - 1))) {
        finish--;
    }
    *finish = '\0';
    if (begin != title) {
        memmove(title, begin, strlen(begin) + 1);
    }
    return title;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <url>\n", argv[0]);
        return 1;
    }
    curl_global_init(CURL_GLOBAL_DEFAULT);
    CURL *curl = curl_easy_init();
    if (!curl) {
        curl_global_cleanup();
        return 1;
    }
    struct Buffer buffer = {0};
    buffer.data = malloc(1);
    if (!buffer.data) {
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return 1;
    }
    curl_easy_setopt(curl, CURLOPT_URL, argv[1]);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");
    CURLcode res