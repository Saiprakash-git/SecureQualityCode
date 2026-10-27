#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

struct HttpRequest {
    std::string method;
    std::map<std::string, std::string> headers;
    std::string path;
};

struct CorsPolicy {
    bool allowAnyOrigin = false;
    std::vector<std::string> allowedOrigins;
    std::vector<std::string> allowedMethods;
    std::vector<std::string> allowedHeaders;
    std::vector<std::string> exposedHeaders;
    bool allowCredentials = false;
    int maxAgeSeconds = 600;
};

struct CorsResult {
    int status = 200;
    bool proceedToHandler = true;
    bool preflight = false;
    std::map<std::string, std::string> headers;
};

static std::string toLower(const std::string& s) {
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return r;
}

static std::string trim(const std::string& s) {
    size_t b = 0, e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return s.substr(b, e - b);
}

static std::vector<std::string> splitCsv(const std::string& s) {
    std::vector<std::string> out;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, ',')) {
        std::string t = trim(item);
        if (!t.empty()) out.push_back(t);
    }
    return out;
}

static std::string joinCsv(const std::vector<std::string>& v) {
    std::string r;
    for (size_t i = 0; i < v.size(); ++i) {
        if (i) r += ", ";
        r += v[i];
    }
    return r;
}

static std::map<std::string, std::string> normalizeHeaders(const std::map<std::string, std::string>& in) {
    std::map<std::string, std::string> out;
    for (const auto& kv : in) out[toLower(kv.first)] = kv.second;
    return out;
}

static bool hasHeader(const HttpRequest& req, const std::string& name) {
    return req.headers.count(toLower(name)) > 0;
}

static std::string getHeader(const HttpRequest& req, const std::string& name) {
    auto it = req.headers.find(toLower(name));
    return it == req.headers.end() ? std::string() : it->second;
}

static bool equalsIgnoreCase(const std::string& a, const std::string& b) {
    return toLower(a) == toLower(b);
}

static bool originAllowed(const CorsPolicy& p, const std::string& origin) {
    if (p.allowAnyOrigin) return true;
    for (const auto& o : p.allowedOrigins) {
        if (o == origin) return true;
    }
    return false;
}

static bool methodAllowed(const CorsPolicy& p, const std::string& method) {
    for (const auto& m : p.allowedMethods) {
        if (m == method) return true;
    }
    return false;
}

static bool headersAllowed(const CorsPolicy& p, const std::vector<std::string>& requested) {
    for (const auto& h : requested) {
        bool found = false;
        for (const auto& a : p.allowedHeaders) {
            if (equalsIgnoreCase(a, h)) { found = true; break; }
        }
        if (!found) return false;
    }
    return true;
}

static void applyOriginHeaders(const CorsPolicy& p, const std::string& origin, std::map<std::string, std::string>& h) {
    if (p.allowAnyOrigin && !p.allowCredentials) {
        h["Access-Control-Allow-Origin"] = "*";
    } else {
        h["Access-Control-Allow-Origin"] = origin;
    }
    h["Vary"] = "Origin";
    if (p.allowCredentials) {
        h["Access-Control-Allow-Credentials"] = "true";
    }
}

CorsResult evaluateCors(const CorsPolicy& policy, const HttpRequest& req) {
    CorsResult result;

    if (!hasHeader(req, "Origin")) {
        // Not a CORS request; pass through without CORS headers.
        return result;
    }

    std::string origin = getHeader(req, "Origin");
    bool isPreflight = (req.method == "OPTIONS") && hasHeader(req, "Access-Control-Request-Method");

    if (isPreflight) {
        result.preflight = true;
        result.proceedToHandler = false;

        if (!originAllowed(policy, origin)) {
            result.status = 403;
            return result;
        }

        std::string reqMethod = trim(getHeader(req, "Access-Control-Request-Method"));
        if (!methodAllowed(policy, reqMethod)) {
            result.status = 403;
            return result;
        }

        std::vector<std::string> reqHeaders = splitCsv(getHeader(req, "Access-Control-Request-Headers"));
        if (!headersAllowed(policy, reqHeaders)) {
            result.status = 403;
            return result;
        }

        applyOriginHeaders(policy, origin, result.headers);
        result.headers["Access-Control-Allow-Methods"] = joinCsv(policy.allowedMethods);
        if (!reqHeaders.empty()) {
            result.headers["Access-Control-Allow-Headers"] = joinCsv(reqHeaders);
            result.headers["Vary"] = "Origin, Access-Control-Request-Headers, Access-Control-Request-Method";
        } else {
            result.headers["Vary"] = "Origin, Access-Control-Request-Method";
        }
        if (!policy.allowedHeaders.empty() && policy.allowAnyOrigin == false) {
            result.headers["Access-Control-Allow-Headers"] = joinCsv(reqHeaders);
        }
        result.headers["Access-Control-Max-Age"] = std::to_string(policy.maxAgeSeconds);
        result.status = 204;
        return result;
    }

    // Actual cross-origin request
    if (!originAllowed(policy, origin)) {
        // Do not add CORS headers; the browser will block the response.
        result.proceedToHandler = true;
        return result;
    }

    applyOriginHeaders(policy, origin, result.headers);
    if (!policy.exposedHeaders.empty()) {
        result.headers["Access-Control-Expose-Headers"] = joinCsv(policy.exposedHeaders);
    }
    result.proceedToHandler = true;
    return result;
}

std::string handleApi(const HttpRequest& req) {
    if (req.path == "/api/items" && req.method == "GET") {
        return "{\"items\":[1,2,3]}";
    }
    return "{\"error\":\"not found\"}";
}

static void printResult(const std::string& title, const CorsResult& r) {
    std::cout << "== " << title << " ==\n";
    std::cout << "Status: " << r.status << "\n";
    std::cout << "Proceed to handler: " << (r.proceedToHandler ? "yes" : "no") << "\n";
    std::cout << "Preflight: " << (r.preflight ? "yes" : "no") << "\n";
    for (const auto& kv : r.headers) {
        std::cout << kv.first << ": " << kv.second << "\n";
    }
    std::cout << "\n";
}

int main() {
    CorsPolicy policy;
    policy.allowedOrigins = {"https://app.example.com"};
    policy.allowedMethods = {"GET", "POST", "PUT", "DELETE"};
    policy.allowedHeaders = {"Content-Type", "Authorization"};
    policy.exposedHeaders = {"X-Request-Id"};
    policy.allowCredentials = true;
    policy.maxAgeSeconds = 600;

    HttpRequest preflight;
    preflight.method = "OPTIONS";
    preflight.path = "/api/items";
    preflight.headers = normalizeHeaders({
        {"Origin", "https://app.example.com"},
        {"Access-Control-Request-Method", "PUT"},
        {"Access-Control-Request-Headers", "content-type, authorization"}
    });
    printResult("Allowed preflight", evaluateCors(policy, preflight));

    HttpRequest badPreflight = preflight;
    badPreflight.headers = normalizeHeaders({
        {"Origin", "https://evil.example.net"},
        {"Access-Control-Request-Method", "PUT"}
    });
    printResult("Disallowed origin preflight", evaluateCors(policy, badPreflight));

    HttpRequest actual;
    actual.method = "GET";
    actual.path = "/api/items";
    actual.headers = normalizeHeaders({
        {"Origin", "https://app.example.com"}
    });
    CorsResult ar = evaluateCors(policy, actual);
    printResult("Allowed actual request", ar);
    if (