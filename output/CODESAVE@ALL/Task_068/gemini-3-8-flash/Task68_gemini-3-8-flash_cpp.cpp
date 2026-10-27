#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <ldap.h>

struct LdapAttribute {
    std::string name;
    std::vector<std::string> values;
};

struct LdapEntry {
    std::string dn;
    std::vector<LdapAttribute> attributes;
};

std::string escapeLdapFilter(const std::string& input) {
    std::ostringstream escaped;
    for (char c : input) {
        switch (c) {
            case '*':
                escaped << "\\2a";
                break;
            case '(':
                escaped << "\\28";
                break;
            case ')':
                escaped << "\\29";
                break;
            case '\\':
                escaped << "\\5c";
                break;
            case '\0':
                escaped << "\\00";
                break;
            default:
                escaped << c;
                break;
        }
    }
    return escaped.str();
}

std::string constructBaseDn(const std::string& dcInput) {
    if (dcInput.find('=') != std::string::npos) {
        return dcInput;
    }

    std::ostringstream baseDn;
    std::istringstream tokenStream(dcInput);
    std::string token;
    bool first = true;

    while (std::getline(tokenStream, token, '.')) {
        if (!token.empty()) {
            if (!first) {
                baseDn << ",";
            }
            baseDn << "dc=" << token;
            first = false;
        }
    }
    return baseDn.str();
}

std::string constructUserFilter(const std::string& username) {
    std::string safeUsername = escapeLdapFilter(username);
    return "(|(uid=" + safeUsername + ")(sAMAccountName=" + safeUsername + ")(cn=" + safeUsername + "))";
}

std::vector<LdapEntry> searchLdapUser(const std::string& host, int port, const std::string& baseDn, const std::string& filter) {
    std::vector<LdapEntry> entries;
    LDAP* ld = nullptr;
    std::string uri = "ldap://" + host + ":" + std::to_string(port);

    int rc = ldap_initialize(&ld, uri.c_str());
    if (rc != LDAP_SUCCESS) {
        std::cerr << "ldap_initialize failed: " << ldap_err2string(rc) << std::endl;
        return entries;
    }

    int version = LDAP_VERSION3;
    ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &version);

    rc = ldap_simple_bind_s(ld, nullptr, nullptr);
    if (rc != LDAP_SUCCESS) {
        std::cerr << "ldap_simple_bind_s failed: " << ldap_err2string(rc) << std::endl;
        ldap_unbind_ext_s(ld, nullptr, nullptr);
        return entries;
    }

    LDAPMessage* searchResult = nullptr;
    rc = ldap_search_ext_s(ld, baseDn.c_str(), LDAP_SCOPE_SUBTREE, filter.c_str(), nullptr, 0, nullptr, nullptr, nullptr, LDAP_NO_LIMIT, &searchResult);
    if (rc != LDAP_SUCCESS) {
        std::cerr << "ldap_search_ext_s failed: " << ldap_err2string(rc) << std::endl;
        if (searchResult != nullptr) {
            ldap_msgfree(searchResult);
        }
        ldap_unbind_ext_s(ld, nullptr, nullptr);
        return entries;
    }

    for (LDAPMessage* entry = ldap_first_entry(ld, searchResult); entry != nullptr; entry = ldap_next_entry(ld, entry)) {
        LdapEntry record;
        char* dn = ldap_get_dn(ld, entry);
        if (dn != nullptr) {
            record.dn = dn;
            ldap_memfree(dn);
        }

        BerElement* ber = nullptr;
        for (char* attr = ldap_first_attribute(ld, entry, &ber); attr != nullptr; attr = ldap_next_attribute(ld, entry, ber)) {
            LdapAttribute ldapAttr;
            ldapAttr.name = attr;

            struct berval** vals = ldap_get_values_len(ld, entry, attr);
            if (vals != nullptr) {
                for (int i = 0; vals[i] != nullptr; ++i) {
                    ldapAttr.values.emplace_back(vals[i]->bv_val, vals[i]->bv_len);
                }
                ldap_value_free_len(vals);
            }
            record.attributes.push_back(ldapAttr);
            ldap_memfree(attr);
        }

        if (ber != nullptr) {
            ber_free(ber, 0);
        }

        entries.push_back(record);
    }

    ldap_msgfree(searchResult);
    ldap_unbind_ext_s(ld, nullptr, nullptr);
    return entries;
}

void printUserEntries(const std::vector<LdapEntry>& entries) {
    if (entries.empty()) {
        std::cout << "No user records found." << std::endl;
        return;
    }

    for (size_t i = 0; i < entries.size(); ++i) {
        std::cout << "--- Entry " << (i + 1) << " ---" << std::endl;
        std::cout << "DN: " << entries[i].dn << std::endl;
        for (const auto& attr : entries[i].attributes) {
            for (const auto& val : attr.values) {
                std::cout << "  " << attr.name << ": " << val << std::endl;
            }
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <domain_component> <username>" << std::endl;
        std::cerr << "Example: " << argv[0] << " example.com jdoe" << std::endl;
        std::cerr << "Example: " << argv[0] << " dc=example,dc=com jdoe" << std::endl;
        return 1;
    }

    std::string rawDc = argv[1];
    std::string username = argv[2];

    std::string baseDn = constructBaseDn(rawDc);
    std::string filter = constructUserFilter(username);

    std::cout << "Connecting to LDAP server on localhost..." << std::endl;
    std::cout << "Base DN: " << baseDn << std::endl;
    std::cout << "Search Filter: " << filter << std::endl;

    std::vector<LdapEntry> results = searchLdapUser("localhost", 389, baseDn, filter);

    printUserEntries(results);

    return 0;
}