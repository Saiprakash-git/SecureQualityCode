#include <ctype.h>
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *data;
    size_t length;
    size_t capacity;
} Buffer;

static int buffer_append(Buffer *buffer, const char *data, size_t length)
{
    if (length > (size_t)-1 - buffer->length - 1)
        return 0;

    size_t needed = buffer->length + length + 1;
    if (needed > buffer->capacity) {
        size_t capacity = buffer->capacity ? buffer->capacity : 4096;
        while (capacity < needed) {
            if (capacity > (size_t)-1 / 2) {
                capacity = needed;
                break;
            }
            capacity *= 2;
        }

        char *grown = realloc(buffer->data, capacity);
        if (!grown)
            return 0;

        buffer->data = grown;
        buffer->capacity = capacity;
    }

    memcpy(buffer->data + buffer->length, data, length);
    buffer->length += length;
    buffer->data[buffer->length] = '\0';
    return 1;
}

static size_t receive_data(char *data, size_t size, size_t count, void *user_data)
{
    if (size && count > (size_t)-1 / size)
        return 0;

    size_t length = size * count;
    return buffer_append((Buffer *)user_data, data, length) ? length : 0;
}

static int equal_ci(const char *a, const char *b, size_t length)
{
    for (size_t i = 0; i < length; ++i) {
        if (tolower((unsigned char)a[i]) != tolower((unsigned char)b[i]))
            return 0;
    }
    return 1;
}

static const char *find_title_open(const char *html, size_t length, size_t *after_tag)
{
    for (size_t i = 0; i + 6 <= length; ++i) {
        if (html[i] != '<' || !equal_ci(html + i + 1, "title", 5))
            continue;

        size_t boundary = i + 6;
        if (boundary < length &&
            !isspace((unsigned char)html[boundary]) &&
            html[boundary] != '>')
            continue;

        size_t end = boundary;
        while (end < length && html[end] != '>')
            ++end;
        if (end == length)
            return NULL;

        *after_tag = end + 1;
        return html + *after_tag;
    }
    return NULL;
}

static const char *find_title_close(const char *html, size_t length, size_t start,
                                    size_t *close_start)
{
    for (size_t i = start; i + 7 <= length; ++i) {
        if (html[i] != '<' || html[i + 1] != '/' ||
            !equal_ci(html + i + 2, "title", 5))
            continue;

        size_t boundary = i + 7;
        if (boundary < length &&
            !isspace((unsigned char)html[boundary]) &&
            html[boundary] != '>')
            continue;

        *close_start = i;
        return html + i;
    }
    return NULL;
}

static size_t encode_utf8(unsigned long codepoint, char *out)
{
    if (codepoint <= 0x7f) {
        out[0] = (char)codepoint;
        return 1;
    }
    if (codepoint <= 0x7ff) {
        out[0] = (char)(0xc0 | (codepoint >> 6));
        out[1] = (char)(0x80 | (codepoint & 0x3f));
        return 2;
    }
    if (codepoint >= 0xd800 && codepoint <= 0xdfff)
        codepoint = 0xfffd;
    if (codepoint <= 0xffff) {
        out[0] = (char)(0xe0 | (codepoint >> 12));
        out[1] = (char)(0x80 | ((codepoint >> 6) & 0x3f));
        out[2] = (char)(0x80 | (codepoint & 0x3f));
        return 3;
    }
    if (codepoint > 0x10ffff)
        codepoint = 0xfffd;
    out[0] = (char)(0xf0 | (codepoint >> 18));
    out[1] = (char)(0x80 | ((codepoint >> 12) & 0x3f));
    out[2] = (char)(0x80 | ((codepoint >> 6) & 0x3f));
    out[3] = (char)(0x80 | (codepoint & 0x3f));
    return 4;
}

static int named_entity(const char *name, size_t length, unsigned long *value)
{
    struct Entity {
        const char *name;
        unsigned long value;
    };

    static const struct Entity entities[] = {
        {"amp", '&'}, {"lt", '<'}, {"gt", '>'}, {"quot", '"'},
        {"apos", '\''}, {"nbsp", 0xa0}, {"copy", 0xa9},
        {"reg", 0xae}, {"mdash", 0x2014}, {"ndash", 0x2013},
        {"hellip", 0x2026}
    };

    for (size_t i = 0; i < sizeof(entities) / sizeof(entities[0]); ++i) {
        if (strlen(entities[i].name) == length &&
            memcmp(name, entities[i].name, length) == 0) {
            *value = entities[i].value;
            return 1;
        }
    }
    return 0;
}

static char *decode_title(const char *text, size_t length)
{
    if (length > ((size_t)-1 - 1) / 4)
        return NULL;

    char *result = malloc(length * 4 + 1);
    if (!result)
        return NULL;

    size_t out = 0;
    for (size_t i = 0; i < length;) {
        if (text[i] == '&') {
            size_t semicolon = i + 1;
            while (semicolon < length && semicolon - i <= 16 &&
                   text[semicolon] != ';')
                ++semicolon;

            if (semicolon < length && text[semicolon] == ';') {
                unsigned long value = 0;
                int valid = 0;

                if (i + 1 < semicolon && text[i + 1] == '#') {
                    size_t pos = i + 2;
                    int base = 10;
                    if (pos < semicolon &&
                        (text[pos] == 'x' || text[pos] == 'X')) {
                        base = 16;
                        ++pos;
                    }
                    if (pos < semicolon) {
                        valid = 1;
                        for (; pos < semicolon; ++pos) {
                            unsigned char c = (unsigned char)text[pos];
                            int digit;
                            if (c >= '0' && c <= '9')
                                digit = c - '0';
                            else if (base == 16 && c >= 'a' && c <= 'f')
                                digit = c - 'a' + 10;
                            else if (base == 16 && c >= 'A' && c <= 'F')
                                digit = c - 'A' + 10;
                            else {
                                valid = 0;
                                break;
                            }
                            if (value > (0x10ffffUL - (unsigned long)digit) /
                                        (unsigned long)base) {
                                valid = 0;
                                break;
                            }
                            value = value * (unsigned long)base +
                                    (unsigned long)digit;
                        }
                    }
                } else {
                    valid = named_entity(text + i + 1, semicolon - i - 1,
                                         &value);
                }

                if (valid) {
                    char encoded[4];
                    size_t encoded_length = encode_utf8(value, encoded);
                    memcpy(result + out, encoded, encoded_length);
                    out += encoded_length;
                    i = semicolon + 1;
                    continue;
                }
            }
        }

        result[out++] = text[i++];
    }

    while (out > 0 && isspace((unsigned char)result[out - 1]))
        --out;

    size_t first = 0;
    while (first < out && isspace((unsigned char)result[first]))
        ++first;

    if (first > 0) {
        memmove(result, result + first, out - first);
        out -= first;
    }
    result[out] = '\0';
    return result;
}

/* Returns a malloc-allocated page title, or NULL if retrieval or extraction fails. */
char *get_page_title(const char *url)
{
    if (!url)
        return NULL;

    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK)
        return NULL;

    Buffer response = {0};
    CURL *curl = curl_easy_init();
    if (!curl)
        return NULL;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "C-Page-Title-Extractor/1.0");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, receive_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

    CURLcode status = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (status != CURLE_OK) {
        free(response.data);
        return NULL;
    }

    size_t after_open = 0;
    if (!find_title_open(response.data ? response.data : "", response.length,
                         &after_open)) {
        free(response.data);
        return NULL;
    }

    size_t close_start = 0;
    if (!find_title_close(response.data, response.length, after_open,
                          &close_start)) {
        free(response.data);
        return NULL;
    }

    char *title = decode_title(response.data + after_open,
                               close_start - after_open);
    free(response.data);
    return title;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s https://example.com/\n", argv[0]);
        return 1;
    }

    char *title = get_page_title(argv[1]);
    if (!title) {
        fprintf(stderr, "Unable to retrieve page or extract its title.\n");
        return 1;
    }

    puts(title);
    free(title);
    return 0;
}