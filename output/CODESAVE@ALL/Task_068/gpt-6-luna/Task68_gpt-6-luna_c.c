#include <ldap.h>
#include <lber.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct {
    char *data;
    size_t length;
    size_t capacity;
} Buffer;

static int buffer_reserve(Buffer *buffer, size_t extra)
{
    size_t needed;
    size_t capacity;
    char *replacement;

    if (extra > (size_t)-1 - buffer->length - 1)
        return 0;

    needed = buffer->length + extra + 1;
    if (needed <= buffer->capacity)
        return 1;

    capacity = buffer->capacity ? buffer->capacity : 64;
    while (capacity < needed) {
        if (capacity > (size_t)-1 / 2) {
            capacity = needed;
            break;
        }
        capacity *= 2;
    }

    replacement = (char *)realloc(buffer->data, capacity);
    if (replacement == NULL)
        return 0;

    buffer->data = replacement;
    buffer->capacity = capacity;
    return 1;
}

static int buffer_append_n(Buffer *buffer, const char *text, size_t length)
{
    if (!buffer_reserve(buffer, length))
        return 0;

    memcpy(buffer->data + buffer->length, text, length);
    buffer->length += length;
    buffer->data[buffer->length] = '\0';
    return 1;
}

static int buffer_append_char(Buffer *buffer, char value)
{
    return buffer_append_n(buffer, &value, 1);
}

static int buffer_append_string(Buffer *buffer, const char *text)
{
    return buffer_append_n(buffer, text, strlen(text));
}

static char *copy_range(const char *start, size_t length)
{
    char *copy = (char *)malloc(length + 1);
    if (copy == NULL)
        return NULL;

    memcpy(copy, start, length);
    copy[length] = '\0';
    return copy;
}

static int parse_request(const char *request, char **username, char **domain)
{
    const char *separator;
    const char *at_sign;

    *username = NULL;
    *domain = NULL;

    at_sign = strchr(request, '@');
    if (at_sign != NULL) {
        if (at_sign == request || at_sign[1] == '\0' ||
            strchr(at_sign + 1, '@') != NULL)
            return 0;

        *username = copy_range(request, (size_t)(at_sign - request));
        *domain = copy_range(at_sign + 1, strlen(at_sign + 1));
    } else {
        separator = strchr(request, '\\');
        if (separator == NULL || separator == request || separator[1] == '\0' ||
            strchr(separator + 1, '\\') != NULL)
            return 0;

        *domain = copy_range(request, (size_t)(separator - request));
        *username = copy_range(separator + 1, strlen(separator + 1));
    }

    if (*username == NULL || *domain == NULL) {
        free(*username);
        free(*domain);
        *username = NULL;
        *domain = NULL;
        return 0;
    }

    return 1;
}

static int append_filter_escaped(Buffer *buffer, const char *value)
{
    static const char hex[] = "0123456789abcdef";
    const unsigned char *p = (const unsigned char *)value;

    while (*p != '\0') {
        if (*p == '*' || *p == '(' || *p == ')' || *p == '\\') {
            char escaped[3];
            escaped[0] = '\\';
            escaped[1] = hex[*p >> 4];
            escaped[2] = hex[*p & 0x0f];
            if (!buffer_append_n(buffer, escaped, sizeof(escaped)))
                return 0;
        } else if (!buffer_append_char(buffer, (char)*p)) {
            return 0;
        }
        ++p;
    }

    return 1;
}

static int append_dn_escaped(Buffer *buffer, const char *value)
{
    size_t length = strlen(value);
    size_t i;

    for (i = 0; i < length; ++i) {
        unsigned char c = (unsigned char)value[i];
        int needs_escape =
            c == ',' || c == '+' || c == '"' || c == '\\' ||
            c == '<' || c == '>' || c == ';' || c == '=' ||
            ((i == 0) && (c == '#' || c == ' ')) ||
            ((i + 1 == length) && c == ' ');

        if (needs_escape && !buffer_append_char(buffer, '\\'))
            return 0;
        if (!buffer_append_char(buffer, (char)c))
            return 0;
    }

    return 1;
}

static char *make_base_dn(const char *domain)
{
    char *domain_copy;
    char *part;
    char *dot;
    Buffer base = {0, 0, 0};

    domain_copy = copy_range(domain, strlen(domain));
    if (domain_copy == NULL)
        return NULL;

    part = domain_copy;
    while (part != NULL) {
        dot = strchr(part, '.');
        if (dot != NULL)
            *dot = '\0';

        if (*part == '\0')
            goto failure;

        if (base.length != 0 && !buffer_append_char(&base, ','))
            goto failure;
        if (!buffer_append_string(&base, "dc=") ||
            !append_dn_escaped(&base, part))
            goto failure;

        part = dot == NULL ? NULL : dot + 1;
    }

    free(domain_copy);
    return base.data;

failure:
    free(domain_copy);
    free(base.data);
    return NULL;
}

static char *make_filter(const char *username)
{
    Buffer filter = {0, 0, 0};

    if (!buffer_append_string(&filter, "(uid=") ||
        !append_filter_escaped(&filter, username) ||
        !buffer_append_char(&filter, ')')) {
        free(filter.data);
        return NULL;
    }

    return filter.data;
}

static int value_needs_base64(const struct berval *value)
{
    size_t i;

    if (value->bv_len == 0)
        return 0;

    if (value->bv_val[0] == ' ' || value->bv_val[0] == ':' ||
        value->bv_val[0] == '<' ||
        value->bv_val[value->bv_len - 1] == ' ')
        return 1;

    for (i = 0; i < value->bv_len; ++i) {
        unsigned char c = (unsigned char)value->bv_val[i];
        if (c < 0x20 || c > 0x7e)
            return 1;
    }

    return 0;
}

static void print_base64(const struct berval *value)
{
    static const char alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t i = 0;

    while (i < value->bv_len) {
        unsigned int a = (unsigned char)value->bv_val[i++];
        unsigned int b = i < value->bv_len
                       ? (unsigned char)value->bv_val[i++] : 0;
        unsigned int c = i < value->bv_len
                       ? (unsigned char)value->bv_val[i++] : 0;
        size_t remaining = value->bv_len - (i >= 3 ? i - 3 : 0);

        putchar(alphabet[(a >> 2) & 0x3f]);
        putchar(alphabet[((a & 0x03) << 4) | ((b >> 4) & 0x0f)]);
        putchar(remaining > 1
                    ? alphabet[((b & 0x0f) << 2) | ((c >> 6) & 0x03)]
                    : '=');
        putchar(remaining > 2 ? alphabet[c & 0x3f] : '=');
    }
}

static void print_ldap_value(const char *attribute, const struct berval *value)
{
    printf("%s%s: ", attribute,
           value_needs_base64(value) ? ":" : "");

    if (value_needs_base64(value))
        print_base64(value);
    else if (value->bv_len != 0)
        fwrite(value->bv_val, 1, value->bv_len, stdout);

    putchar('\n');
}

static int print_entry(LDAP *connection, LDAPMessage *entry)
{
    char *dn;
    BerElement *ber = NULL;
    char *attribute;
    int printed = 0;

    dn = ldap_get_dn(connection, entry);
    if (dn == NULL)
        return 0;

    printf("dn: %s\n", dn);
    ldap_memfree(dn);

    for (attribute = ldap_first_attribute(connection, entry, &ber);
         attribute != NULL;
         attribute = ldap_next_attribute(connection, entry, ber)) {
        struct berval **values = ldap_get_values_len(connection, entry, attribute);
        size_t i;

        if (values != NULL) {
            for (i = 0; values[i] != NULL; ++i) {
                print_ldap_value(attribute, values[i]);
                printed = 1;
            }
            ldap_value_free_len(values);
        }
        ldap_memfree(attribute);
    }

    if (ber != NULL)
        ber_free(ber, 0);

    putchar('\n');
    return printed;
}

int main(int argc, char **argv)
{
    LDAP *connection = NULL;
    LDAPMessage *result = NULL;
    LDAPMessage *entry;
    char *username = NULL;
    char *domain = NULL;
    char *base_dn = NULL;
    char *filter = NULL;
    int version = LDAP_VERSION3;
    int status;
    int found = 0;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s username@domain\n", argv[0]);
        fprintf(stderr, "   or: %s domain\\\\username\n", argv[0]);
        return 2;
    }

    if (!parse_request(argv[1], &username, &domain)) {
        fprintf(stderr, "Invalid request; expected username@domain or domain\\\\username.\n");
        return 2;
    }

    base_dn = make_base_dn(domain);
    filter = make_filter(username);
    if (base_dn == NULL || filter == NULL) {
        fprintf(stderr, "Unable to construct LDAP query.\n");
        free(username);
        free(domain);
        free(base_dn);
        free(filter);
        return 1;
    }

    status = ldap_initialize(&connection, "ldap://127.0.0.1:389");
    if (status != LDAP_SUCCESS) {
        fprintf(stderr, "LDAP initialization failed: %s\n", ldap_err2string(status));
        free(username);
        free(domain);
        free(base_dn);
        free(filter);
        return 1;
    }

    ldap_set_option(connection, LDAP_OPT_PROTOCOL_VERSION, &version);

    status = ldap_simple_bind_s(connection, NULL, NULL);
    if (status != LDAP_SUCCESS) {
        fprintf(stderr, "LDAP bind failed: %s\n", ldap_err2string(status));
        ldap_unbind_ext_s(connection, NULL, NULL);
        free(username);
        free(domain);
        free(base_dn);
        free(filter);
        return 1;
    }

    status = ldap_search_ext_s