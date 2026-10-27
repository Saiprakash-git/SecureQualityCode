#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ldap.h>

#define LDAP_URI "ldap://localhost:389"

static int build_base_dn(const char *domain, char *out, size_t outsz)
{
    char tmp[512];
    char *tok, *saveptr = NULL;
    size_t used = 0;

    if (!domain || !out || outsz == 0)
        return -1;

    if (strlen(domain) >= sizeof(tmp))
        return -1;

    strcpy(tmp, domain);
    out[0] = '\0';

    for (tok = strtok_r(tmp, ".", &saveptr); tok; tok = strtok_r(NULL, ".", &saveptr)) {
        int n;
        if (used > 0) {
            n = snprintf(out + used, outsz - used, ",");
            if (n < 0 || (size_t)n >= outsz - used)
                return -1;
            used += (size_t)n;
        }
        n = snprintf(out + used, outsz - used, "dc=%s", tok);
        if (n < 0 || (size_t)n >= outsz - used)
            return -1;
        used += (size_t)n;
    }

    return (used > 0) ? 0 : -1;
}

static int escape_filter_value(const char *in, char *out, size_t outsz)
{
    size_t j = 0;

    if (!in || !out || outsz == 0)
        return -1;

    for (size_t i = 0; in[i]; i++) {
        unsigned char c = (unsigned char)in[i];
        const char *rep = NULL;
        char hex[4];

        switch (c) {
        case '*':  rep = "\\2a"; break;
        case '(':  rep = "\\28"; break;
        case ')':  rep = "\\29"; break;
        case '\\': rep = "\\5c"; break;
        case '\0': rep = "\\00"; break;
        default:
            hex[0] = (char)c;
            hex[1] = '\0';
            rep = hex;
            break;
        }

        size_t rl = strlen(rep);
        if (j + rl + 1 > outsz)
            return -1;
        memcpy(out + j, rep, rl);
        j += rl;
    }

    out[j] = '\0';
    return 0;
}

static int build_user_filter(const char *username, char *out, size_t outsz)
{
    char escaped[256];
    int n;

    if (!username || !out || outsz == 0)
        return -1;

    if (escape_filter_value(username, escaped, sizeof(escaped)) != 0)
        return -1;

    n = snprintf(out, outsz, "(&(objectClass=person)(uid=%s))", escaped);
    if (n < 0 || (size_t)n >= outsz)
        return -1;

    return 0;
}

static int search_user(const char *base_dn, const char *filter)
{
    LDAP *ld = NULL;
    LDAPMessage *res = NULL, *entry;
    struct timeval tv = {10, 0};
    int version = LDAP_VERSION3;
    int rc, count;

    rc = ldap_initialize(&ld, LDAP_URI);
    if (rc != LDAP_SUCCESS) {
        fprintf(stderr, "ldap_initialize: %s\n", ldap_err2string(rc));
        return 1;
    }

    ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &version);

    rc = ldap_simple_bind_s(ld, NULL, NULL);
    if (rc != LDAP_SUCCESS) {
        fprintf(stderr, "LDAP bind failed: %s\n", ldap_err2string(rc));
        ldap_unbind_ext_s(ld, NULL, NULL);
        return 1;
    }

    printf("Base DN: %s\n", base_dn);
    printf("Filter : %s\n", filter);

    rc = ldap_search_ext_s(ld, base_dn, LDAP_SCOPE_SUBTREE, filter,
                           NULL, 0, NULL, NULL, &tv, 0, &res);
    if (rc != LDAP_SUCCESS) {
        fprintf(stderr, "LDAP search failed: %s\n", ldap_err2string(rc));
        if (res)
            ldap_msgfree(res);
        ldap_unbind_ext_s(ld, NULL, NULL);
        return 1;
    }

    count = ldap_count_entries(ld, res);
    if (count <= 0) {
        printf("No user found.\n");
        ldap_msgfree(res);
        ldap_unbind_ext_s(ld, NULL, NULL);
        return 1;
    }

    for (entry = ldap_first_entry(ld, res); entry != NULL;
         entry = ldap_next_entry(ld, entry)) {
        char *dn = ldap_get_dn(ld, entry);
        char *attr;
        BerElement *ber = NULL;

        printf("\nDN: %s\n", dn ? dn : "(unknown)");

        for (attr = ldap_first_attribute(ld, entry, &ber); attr != NULL;
             attr = ldap_next_attribute(ld, entry, ber)) {
            struct berval **vals = ldap_get_values_len(ld, entry, attr);
            if (vals) {
                for (int i = 0; vals[i] != NULL; i++) {
                    printf("%s: %.*s\n", attr,
                           (int)vals[i]->bv_len, vals[i]->bv_val);
                }
                ldap_value_free_len(vals);
            }
            ldap_memfree(attr);
        }

        if (ber)
            ber_free(ber, 0);
        if (dn)
            ldap_memfree(dn);
    }

    ldap_msgfree(res);
    ldap_unbind_ext_s(ld, NULL, NULL);
    return 0;
}

static void usage(const char *prog)
{
    fprintf(stderr,
            "Usage:\n"
            "  %s <username>@<domain>\n"
            "  %s <username> <domain>\n"
            "Example:\n"
            "  %s jdoe example.com\n",
            prog, prog, prog);
}

int main(int argc, char *argv[])
{
    char username[256] = {0};
    char domain[256] = {0};
    char base_dn[512];
    char filter[512];
    char *at;

    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }

    at = strchr(argv[1], '@');
    if (at) {
        size_t ulen = (size_t)(at - argv[1]);
        if (ulen == 0 || ulen >= sizeof(username) || strlen(at + 1) >= sizeof(domain)) {
            usage(argv[0]);
            return 1;
        }
        memcpy(username, argv[1], ulen);
        username[ulen] = '\0';
        strcpy(domain, at + 1);
    } else {
        if (argc < 3 || strlen(argv[1]) >= sizeof(username) || strlen(argv[2]) >= sizeof(domain)) {
            usage(argv[0]);
            return 1;
        }
        strcpy(username, argv[1]);
        strcpy(domain, argv[2]);
    }

    if (build_base_dn(domain, base_dn, sizeof(base_dn)) != 0) {
        fprintf(stderr, "Failed to build base DN from domain '%s'\n", domain);
        return 1;
    }

    if (build_user_filter(username, filter, sizeof(filter)) != 0) {
        fprintf(stderr, "Failed to build LDAP filter for user '%s'\n", username);
        return 1;
    }

    return search_user(base_dn, filter);
}