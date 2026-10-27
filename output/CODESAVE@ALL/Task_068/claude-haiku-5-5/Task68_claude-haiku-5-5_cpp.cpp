#include <ldap.h>
#include <cstring>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

static std::string trim(const std::string& s) {
    size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

static std::string urlDecode(const std::string& in) {
    std::string out;
    for (size_t i = 0; i < in.size(); ++i) {
        if (in[i] == '+') {
            out += ' ';
        } else if (in[i] == '%' && i + 2 < in.size() && isxdigit((unsigned char)in[i + 1]) && isxdigit((unsigned char)in[i + 2])) {
            std::string hex = in.substr(i + 1, 2);
            out += static_cast<char>(std::stoi(hex, nullptr, 16));
            i += 2;
        } else {
            out += in[i];
        }
    }
    return out;
}

// Extracts username and dc from a request string such as
// "username=alice&dc=example.com" or "user=alice;dc=dc=example,dc=com"
// or from separate values (username, dc).
static bool extractFromRequest(const std::vector<std::string>& args,
                               std::string& username,
                               std::string& dc) {
    username.clear();
    dc.clear();

    if (args.size() >= 2) {
        username = trim(args[0]);
        dc = trim(args[1]);
        return !username.empty() && !dc.empty();
    }

    if (args.size() == 1) {
        std::string req = args[0];
        std::string token;
        std::vector<std::string> pairs;
        for (char c : req) {
            if (c == '&' || c == ';') {
                pairs.push_back(token);
                token.clear();
            } else {
                token += c;
            }
        }
        pairs.push_back(token);

        for (const auto& p : pairs) {
            size_t eq = p.find('=');
            if (eq == std::string::npos) continue;
            std::string key = trim(p.substr(0, eq));
            std::string val = urlDecode(trim(p.substr(eq + 1)));
            if (key == "username" || key == "user" || key == "uid") {
                username = val;
            } else if (key == "dc" || key == "domain") {
                dc = val;
            }
        }
        return !username.empty() && !dc.empty();
    }
    return false;
}

// Converts "example.com" or "dc=example,dc=com" into "dc=example,dc=com"
static std::string buildBaseDN(const std::string& dcInput) {
    if (dcInput.find('=') != std::string::npos) {
        return dcInput;
    }
    std::string result;
    std::string part;
    std::stringstream ss(dcInput);
    while (std::getline(ss, part, '.')) {
        part = trim(part);
        if (part.empty()) continue;
        if (!result.empty()) result += ",";
        result += "dc=" + part;
    }
    return result;
}

// RFC 4515 filter value escaping
static std::string escapeFilterValue(const std::string& in) {
    std::string out;
    for (unsigned char c : in) {
        switch (c) {
            case '*': out += "\\2a"; break;
            case '(': out += "\\28"; break;
            case ')': out += "\\29"; break;
            case '\\': out += "\\5c"; break;
            case '\0': out += "\\00"; break;
            default: out += static_cast<char>(c);
        }
    }
    return out;
}

static std::string buildFilter(const std::string& username) {
    return "(uid=" + escapeFilterValue(username) + ")";
}

// Searches LDAP on localhost and returns formatted user information.
static int searchUser(const std::string& baseDN,
                      const std::string& filter,
                      std::string& output) {
    LDAP* ld = nullptr;
    int rc = ldap_initialize(&ld, "ldap://localhost:389");
    if (rc != LDAP_SUCCESS || ld == nullptr) {
        output = "Failed to initialize LDAP: " + std::string(ldap_err2string(rc));
        return 1;
    }

    int version = LDAP_VERSION3;
    ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &version);

    const char* bindDn = std::getenv("LDAP_BIND_DN");
    const char* bindPw = std::getenv("LDAP_BIND_PW");

    rc = ldap_simple_bind_s(ld, bindDn, bindPw);
    if (rc != LDAP_SUCCESS) {
        output = "LDAP bind failed: " + std::string(ldap_err2string(rc));
        ldap_unbind_ext_s(ld, nullptr, nullptr);
        return 1;
    }

    LDAPMessage* res = nullptr;
    struct timeval tv = {10, 0};
    rc = ldap_search_ext_s(ld, baseDN.c_str(), LDAP_SCOPE_SUBTREE,
                           filter.c_str(), nullptr, 0, nullptr, nullptr,
                           &tv, 0, &res);
    if (rc != LDAP_SUCCESS) {
        output = "LDAP search failed: " + std::string(ldap_err2string(rc));
        if (res) ldap_msgfree(res);
        ldap_unbind_ext_s(ld, nullptr, nullptr);
        return 1;
    }

    std::ostringstream os;
    int count = ldap_count_entries(ld, res);
    if (count <= 0) {
        os << "No user found for filter " << filter << " under base " << baseDN << "\n";
    } else {
        int entryIndex = 0;
        for (LDAPMessage* entry = ldap_first_entry(ld, res);
             entry != nullptr;
             entry = ldap_next_entry(ld, entry)) {
            ++entryIndex;
            char* dn = ldap_get_dn(ld, entry);
            os << "Entry " << entryIndex << "\n";
            os << "  dn: " << (dn ? dn : "") << "\n";
            if (dn) ldap_memfree(dn);

            BerElement* ber = nullptr;
            for (char* attr = ldap_first_attribute(ld, entry, &ber);
                 attr != nullptr;
                 attr = ldap_next_attribute(ld, entry, ber)) {
                berval** vals = ldap_get_values_len(ld, entry, attr);
                if (vals) {
                    for (int i = 0; vals[i] != nullptr; ++i) {
                        os << "  " << attr << ": "
                           << std::string(vals[i]->bv_val, vals[i]->bv_len) << "\n";
                    }
                    ldap_value_free_len(vals);
                }
                ldap_memfree(attr);
            }
            if (ber) ber_free(ber, 0);
        }
    }

    output = os.str();
    ldap_msgfree(res);
    ldap_unbind_ext_s(ld, nullptr, nullptr);
    return 0;
}

int main(int argc, char* argv[]) {
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        args.push_back(argv[i]);
    }

    if (args.empty()) {
        std::cerr << "Usage: " << argv[0] << " \"username=<user>&dc=<domain>\"\n"
                  << "   or: " << argv[0] << " <username> <dc>\n";
        return 1;
    }

    std::string username, dcInput;
    if (!extractFromRequest(args, username, dcInput)) {
        std::cerr << "Error: could not extract username and dc from request.\n";
        return 1;
    }

    std::string baseDN = buildBaseDN(dcInput);
    std::string filter = buildFilter(username);

    std::cout << "Base DN: " << baseDN << "\n";
    std::cout << "Filter:  " << filter << "\n";

    std::string output;
    int rc = searchUser(baseDN, filter, output);
    std::cout << output;
    return rc;
}