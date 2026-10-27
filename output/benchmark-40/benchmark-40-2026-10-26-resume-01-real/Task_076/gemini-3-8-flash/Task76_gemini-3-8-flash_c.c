#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <curl/curl.h>

struct MemoryBuffer {
    char *data;
    size_t size;
};

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryBuffer *mem = (struct MemoryBuffer *)userp;

    char *ptr = realloc(mem->data, mem->size + realsize + 1);
    if (!ptr) {
        return 0;
    }

    mem->data = ptr;
    memcpy(&(mem->data[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->data[mem->size] = 0;

    return realsize;
}

static const char *case_insensitive_search(const char *haystack, const char *needle) {
    if (!haystack || !needle) return NULL;
    size_t needle_len = strlen(needle);
    if (needle_len == 0) return haystack;

    while (*haystack) {
        if (strncasecmp(haystack, needle, needle_len) == 0) {
            return haystack;
        }
        haystack++;
    }
    return NULL;
}

char *fetch_url(const char *url) {
    CURL *curl_handle;
    CURLcode res;
    struct MemoryBuffer chunk;

    chunk.data = malloc(1);
    chunk.size = 0;

    curl_global_init(CURL_GLOBAL_ALL);
    curl_handle = curl_easy_init();

    if (!curl_handle) {
        free(chunk.data);
        curl_global_cleanup();
        return NULL;
    }

    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "Mozilla/5.0 (compatible; TitleFetcher/1.0)");
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 2L);

    res = curl_easy_perform(curl_handle);

    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();

    if (res != CURLE_OK) {
        free(chunk.data);
        return NULL;
    }

    return chunk.data;
}

char *extract_title(const char *html) {
    if (!html) return NULL;

    const char *title_open = case_insensitive_search(html, "<title");
    if (!title_open) return NULL;

    const char *content_start = strchr(title_open, '>');
    if (!content_start) return NULL;
    content_start++;

    const char *content_end = case_insensitive_search(content_start, "</title>");
    if (!content_end) return NULL;

    while (content_start < content_end && isspace((unsigned char)*content_start)) {
        content_start++;
    }

    while (content_end > content_start && isspace((unsigned char)*(content_end - 1))) {
        content_end--;
    }

    size_t title_len = content_end - content_start;
    char *title = malloc(title_len + 1);
    if (!title) return NULL;

    memcpy(title, content_start, title_len);
    title[title_len] = '\0';

    return title;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <HTTPS URL>\n", argv[0]);
        return 1;
    }

    const char *url = argv[1];
    char *html = fetch_url(url);
    if (!html) {
        fprintf(stderr, "Error: Failed to fetch webpage content.\n");
        return 1;
    }

    char *title = extract_title(html);
    if (title) {
        printf("%s\n", title);
        free(title);
    } else {
        printf("No title found.\n");
    }

    free(html);
    return 0;
}