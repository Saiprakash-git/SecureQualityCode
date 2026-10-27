#include <ldap.h>

#include <cctype>
#include <iostream>
#include <string>
#include <vector>

static std::string escapeFilterValue(const std::string& value) {
    static const char hex[] = "0123456789abcdef";
    std::string escaped;
    for (unsigned char c : value) {
        if (c == '*' || c == '(' || c == ')' || c == '\\' || c == '\0') {
            escaped.push_back('\\');
            escaped.push_back(hex[c >> 4]);
            escaped.push_back(hex[c & 0x0f]);
        } else {
            escaped.push_back(static_cast<char>(c));
        }
    }
    return escaped;
}

static std::string escapeDnValue(const std::string& value) {
    std::string escaped;
    for (std::size_t i = 0; i < value.size(); ++i) {
        const char c = value[i];
        const bool edgeSpace = c == ' ' && (i == 0 || i + 1 == value.size());
        if (edgeSpace || c == ',' || c == '+' || c == '"' || c == '\\' ||
            c == '<' || c == '>' || c == ';' || c == '=' ||
            (c == '#' && i == 0)) {
            escaped.push_back('\\');
        }
        escaped.push_back(c);
    }
    return escaped;
}

static std::string encodeValue(const struct berval* value) {
    static const char hex[] = "0123456789abcdef";
    std::string encoded;
    for (ber_len_t i = 0; i < value->bv_len; ++i) {
        const unsigned char c = static_cast<unsigned char>(value->bv_val[i]);
        if (std::isprint(c) && c != '\\') {
            encoded.push_back(static_cast<char>(c));
        } else {
            encoded += "\\x";
            encoded.push_back(hex[c >> 4]);
            encoded.push_back(hex[c & 0x0f]);
        }
    }
    return encoded;
}

static bool parseRequest(const std::string& request, std::string& username,
                         std::string& domain) {
    const std::size_t at = request.rfind('@');
    if (at != std::string::npos) {
        username = request.substr(0, at);
        domain = request.substr(at + 1);
    } else {
        const std::size_t slash = request.find('\\');
        if (slash == std::string::npos) {
            return false;
        }
        domain = request.substr(0, slash);
        username = request.substr(slash + 1);
    }

    if (username.empty() || domain.empty()) {
        return false;
    }

    std::size_t start = 0;
    while (start < domain.size()) {
        const std::size_t end = domain.find('.', start);
        const std::size_t length =
            (end == std::string::npos ? domain.size() : end) - start;
        if (length == 0) {
            return false;
        }
        start = end == std::string::npos ? domain.size() : end + 1;
    }
    return true;
}

static std::string makeBaseDn(const std::string& domain) {
    std::string baseDn;
    std::size_t start = 0;
    while (start < domain.size()) {
        const std::size_t end = domain.find('.', start);
        const std::string label =
            domain.substr(start, end == std::string::npos
                                     ? std::string::npos
                                     : end - start);
        if (!baseDn.empty()) {
            baseDn.push_back(',');
        }
        baseDn += "dc=" + escapeDnValue(label);
        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
    }
    return baseDn;
}

static std::string processRequest(const std::string& request) {
    std::string username;
    std::string domain;
    if (!parseRequest(request, username, domain)) {
        return "Invalid request. Expected username@domain or domain\\username.\n";
    }

    const std::string baseDn = makeBaseDn(domain);
    const std::string filter = "(uid=" + escapeFilterValue(username) + ")";

    LDAP* ldap = nullptr;
    int rc = ldap_initialize(&ldap, "ldap://localhost:389");
    if (rc != LDAP_SUCCESS || ldap == nullptr) {
        return "LDAP connection failed: " + std::string(ldap_err2string(rc)) + "\n";
    }

    int version = LDAP_VERSION3;
    rc = ldap_set_option(ldap, LDAP_OPT_PROTOCOL_VERSION, &version);
    if (rc != LDAP_OPT_SUCCESS) {
        const std::string error = ldap_err2string(rc);
        ldap_unbind_ext_s(ldap, nullptr, nullptr);
        return "LDAP setup failed: " + error + "\n";
    }

    struct berval credentials {};
    rc = ldap_sasl_bind_s(ldap, nullptr, LDAP_SASL_SIMPLE, &credentials,
                          nullptr, nullptr, nullptr);
    if (rc != LDAP_SUCCESS) {
        const std::string error = ldap_err2string(rc);
        ldap_unbind_ext_s(ldap, nullptr, nullptr);
        return "LDAP bind failed: " + error + "\n";
    }

    LDAPMessage* results = nullptr;
    rc = ldap_search_ext_s(ldap, baseDn.c_str(), LDAP_SCOPE_SUBTREE,
                           filter.c_str(), nullptr, 0, nullptr, nullptr, nullptr,
                           LDAP_NO_LIMIT, &results);
    if (rc != LDAP_SUCCESS) {
        const std::string error = ldap_err2string(rc);
        if (results != nullptr) {
            ldap_msgfree(results);
        }
        ldap_unbind_ext_s(ldap, nullptr, nullptr);
        return "LDAP search failed: " + error + "\n";
    }

    std::string output;
    LDAPMessage* entry = ldap_first_entry(ldap, results);
    if (entry == nullptr) {
        output = "No user found.\n";
    }

    for (; entry != nullptr; entry = ldap_next_entry(ldap, entry)) {
        char* dn = ldap_get_dn(ldap, entry);
        output += "dn: ";
        output += dn != nullptr ? dn : "";
        output.push_back('\n');
        if (dn != nullptr) {
            ldap_memfree(dn);
        }

        BerElement* ber = nullptr;
        char* attribute = ldap_first_attribute(ldap, entry, &ber);
        while (attribute != nullptr) {
            struct berval** values = ldap_get_values_len(ldap, entry, attribute);
            if (values != nullptr) {
                for (std::size_t i = 0; values[i] != nullptr; ++i) {
                    output += attribute;
                    output += ": ";
                    output += encodeValue(values[i]);
                    output.push_back('\n');
                }
                ldap_value_free_len(values);
            }
            ldap_memfree(attribute);
            attribute = ldap_next_attribute(ldap, entry, ber);
        }
        if (ber != nullptr) {
            ber_free(ber, 0);
        }
        output.push_back('\n');
    }

    ldap_msgfree(results);
    ldap_unbind_ext_s(ldap, nullptr, nullptr);
    return output;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " username@domain\n";
        return 1;
    }

    std::cout << processRequest(argv[1]);
    return 0;
}