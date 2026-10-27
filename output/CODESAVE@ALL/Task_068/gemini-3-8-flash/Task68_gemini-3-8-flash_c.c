#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <ldap.h>

static char *escape_ldap_filter(const char *src) {
    if (!src) return NULL;
    size_t len = 0;
    for (const char *p = src; *p; p++) {
        switch (*p) {
            case '*':
            case '(':
            case ')':
            case '\\':
            case '/':
                len += 3;
                break;
            default:
                len += 1;
                break;
        }
    }

    char *dest = (char *)malloc(len + 1);
    if (!dest) return NULL;

    char *q = dest;
    for (const char *p = src; *p; p++) {
        switch (*p) {
            case '*':
                memcpy(q, "\\2a", 3);
                q += 3;
                break;
            case '(':
                memcpy(q, "\\28", 3);
                q += 3;
                break;
            case ')':
                memcpy(q, "\\29", 3);
                q += 3;
                break;
            case '\\':
                memcpy(q, "\\5c", 3);
                q += 3;
                break;
            case '/':
                memcpy(q, "\\2f", 3);
                q += 3;
                break;
            default:
                *q++ = *p;
                break;
        }
    }
    *q = '\0';
    return dest;
}

static char *build_base_dn(const char *domain) {
    if (!domain || strlen(domain) == 0) return NULL;

    if (strchr(domain, '=') != NULL) {
        return strdup(domain);
    }

    char *temp = strdup(domain);
    if (!temp) return NULL;

    size_t dn_size = strlen(domain) * 4 + 1;
    char *base_dn = (char *)malloc(dn_size);
    if (!base_dn) {
        free(temp);
        return NULL;
    }
    base_dn[0] = '\0';

    char *saveptr = NULL;
    char *token = strtok_r(temp, ".", &saveptr);
    int first = 1;

    while (token != NULL) {
        if (!first) {
            strcat(base_dn, ",");
        }
        strcat(base_dn, "dc=");
        strcat(base_dn, token);
        first = 0;
        token = strtok_r(NULL, ".", &saveptr);
    }

    free(temp);
    return base_dn;
}

static int execute_ldap_search(const char *uri, const char *base_dn, const char *username) {
    LDAP *ld = NULL;
    int rc;
    int version = LDAP_VERSION3;

    rc = ldap_initialize(&ld, uri);
    if (rc != LDAP_SUCCESS) {
        fprintf(stderr, "ldap_initialize failed: %s\n", ldap_err2string(rc));
        return 1;
    }

    rc = ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &version);
    if (rc != LDAP_SUCCESS) {
        fprintf(stderr, "ldap_set_option failed: %s\n", ldap_err2string(rc));
        ldap_unbind_ext_s(ld, NULL, NULL);
        return 1;
    }

    struct berval cred;
    cred.bv_val = NULL;
    cred.bv_len = 0;
    rc = ldap_sasl_bind_s(ld, NULL, LDAP_SASL_SIMPLE, &cred, NULL, NULL, NULL);
    if (rc != LDAP_SUCCESS) {
        fprintf(stderr, "ldap_sasl_bind_s failed: %s\n", ldap_err2string(rc));
        ldap_unbind_ext_s(ld, NULL, NULL);
        return 1;
    }

    char *escaped_user = escape_ldap_filter(username);
    if (!escaped_user) {
        ldap_unbind_ext_s(ld, NULL, NULL);
        return 1;
    }

    char filter[512];
    snprintf(filter, sizeof(filter), "(|(uid=%s)(sAMAccountName=%s)(cn=%s))", 
             escaped_user, escaped_user, escaped_user);
    free(escaped_user);

    LDAPMessage *result = NULL;
    rc = ldap_search_ext_s(ld, base_dn, LDAP_SCOPE_SUBTREE, filter, NULL, 0, 
                           NULL, NULL, NULL, LDAP_NO_LIMIT, &result);
    if (rc != LDAP_SUCCESS) {
        fprintf(stderr, "ldap_search_ext_s failed: %s\n", ldap_err2string(rc));
        ldap_unbind_ext_s(ld, NULL, NULL);
        return 1;
    }

    int entry_count = ldap_count_entries(ld, result);
    if (entry_count == 0) {
        printf("No entries found matching query: %s\n", filter);
    } else {
        LDAPMessage *entry;
        for (entry = ldap_first_entry(ld, result); entry != NULL; entry = ldap_next_entry(ld, entry)) {
            char *dn = ldap_get_dn(ld, entry);
            if (dn) {
                printf("DN: %s\n", dn);
                ldap_memfree(dn);
            }

            BerElement *ber = NULL;
            char *attribute;
            for (attribute = ldap_first_attribute(ld, entry, &ber); attribute != NULL; 
                 attribute = ldap_next_attribute(ld, entry, ber)) {
                struct berval **vals = ldap_get_values_len(ld, entry, attribute);
                if (vals) {
                    for (int i = 0; vals[i] != NULL; i++) {
                        printf("  %s: %.*s\n", attribute, (int)vals[i]->bv_len, vals[i]->bv_val);
                    }
                    ldap_value_free_len(vals);
                }
                ldap_memfree(attribute);
            }
            if (ber) {
                ber_free(ber, 0);
            }
            printf("\n");
        }
    }

    if (result) {
        ldap_msgfree(result);
    }
    ldap_unbind_ext_s(ld, NULL, NULL);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <domain|base_dn> <username>\n", argv[0]);
        fprintf(stderr, "Example: %s example.com jdoe\n", argv[0]);
        fprintf(stderr, "Example: %s dc=example,dc=com jdoe\n", argv[0]);
        return 1;
    }

    const char *raw_domain = argv[1];
    const char *username = argv[2];

    char *base_dn = build_base_dn(raw_domain);
    if (!base_dn) {
        fprintf(stderr, "Failed to parse domain/base DN.\n");
        return 1;
    }

    const char *ldap_uri = "ldap://localhost:389";
    printf("Connecting to: %s\n", ldap_uri);
    printf("Base DN: %s\n", base_dn);
    printf("Searching for user: %s\n\n", username);

    int status = execute_ldap_search(ldap_uri, base_dn, username);

    free(base_dn);
    return status;
}