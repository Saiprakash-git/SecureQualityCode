#include <iostream>
#include <string>
#include <vector>
#include <ldap.h>

std::string constructBaseDN(const std::vector<std::string>& dcComponents) {
    std::string baseDN;
    for (size_t i = 0; i < dcComponents.size(); ++i) {
        if (i > 0) baseDN += ",";
        baseDN += "dc=" + dcComponents[i];
    }
    return baseDN;
}

std::string constructFilter(const std::string& username) {
    return "(uid=" + username + ")";
}

int searchUser(const std::string& username, const std::vector<std::string>& dcComponents) {
    std::string baseDN = constructBaseDN(dcComponents);
    std::string filter = constructFilter(username);

    LDAP* ld = nullptr;
    int rc = ldap_initialize(&ld, "ldap://localhost");
    if (rc != LDAP_SUCCESS) {
        std::cerr << "ldap_initialize failed: " << ldap_err2string(rc) << std::endl;
        return 1;
    }

    int version = LDAP_VERSION3;
    ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &version);

    rc = ldap_simple_bind(