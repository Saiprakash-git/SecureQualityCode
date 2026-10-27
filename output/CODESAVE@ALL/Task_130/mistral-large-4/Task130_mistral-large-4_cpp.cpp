#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <sstream>

class CorsConfig {
public:
    std::string allowedOrigin;
    std::vector<std::string> allowedMethods;
    std::vector<std::string> allowedHeaders;
    bool allowCredentials;
    int maxAge;

    CorsConfig() : allowedOrigin("*"), allowCredentials(false), maxAge(86400) {
        allowedMethods = {"GET", "POST", "PUT", "DELETE", "OPTIONS"};
        allowedHeaders = {"Content-Type", "Authorization", "X-Requested-With"};
    }
};

class HttpRequest {
public:
    std::string method;
    std::string path;
    std::map<std::string, std::string> headers;
    std::string body;

    std::string getHeader(const std::string& name) const {
        auto it = headers.find(name);
        if (it != headers.end()) return it->second;
        return "";
    }
};

class HttpResponse {
public:
    int statusCode;
    std::map<std::string, std::string> headers;
    std::string body;

    HttpResponse() : statusCode(200) {}

    void setHeader(const std::string& name, const std::string& value) {
        headers[name] = value;
    }

    std::string toString() const {
        std::ostringstream oss;
        oss << "HTTP/1.1 " << statusCode << " OK\r\n";
        for (const auto& h : headers) {
            oss << h.first << ": " << h.second << "\r\n";
        }
        oss << "\r\n" << body;
        return oss.str();
    }
};

class CorsApi {
private:
    CorsConfig config;

    bool isOriginAllowed(const std::string& origin) const {
        if (config.allowedOrigin == "*") return true;
        return origin == config.allowedOrigin;
    }

    std::string joinVector(const std::vector<std::string>& vec, const std::string& delim) const {
        std::ostringstream oss;
        for (size_t i = 0; i < vec.size(); ++i) {
            if (i > 0) oss << delim;
            oss << vec[i];
        }
        return oss.str();
    }

public:
    CorsApi(const CorsConfig& cfg) : config(cfg) {}

    HttpResponse handlePreflight(const HttpRequest& request) {
        HttpResponse response;
        response.statusCode = 204;

        std::string origin = request.getHeader("Origin");
        std::string requestMethod = request.getHeader("Access-Control-Request-Method");

        if (!origin.empty() && isOriginAllowed(origin)) {
            response.setHeader("Access-Control-Allow-Origin", origin);
            response.setHeader("Access-Control-Allow-Methods", joinVector(config.allowedMethods, ", "));
            response.setHeader("Access-Control-Allow-Headers", joinVector(config.allowedHeaders, ", "));
            response.setHeader("Access-Control-Max-Age", std::to_string(config.maxAge));

            if (config.allowCredentials) {
                response.setHeader("Access-Control-Allow-Credentials", "true");
            }

            response.setHeader("Vary", "Origin");
        }

        return response;
    }

    HttpResponse handleRequest(const HttpRequest& request) {
        HttpResponse response;

        if (request.method == "OPTIONS") {
            return handlePreflight(request);
        }

        std::string origin = request.getHeader("Origin");
        if (!origin.empty() && isOriginAllowed(origin)) {
            response.setHeader("Access-Control-Allow-Origin", origin);
            if (config.allowCredentials) {
                response.setHeader("Access-Control-Allow-Credentials", "true");
            }
            response.setHeader("Vary", "Origin");
        }

        if (request.path == "/api/data" && request.method == "GET") {
            response.statusCode = 200;
            response.setHeader("Content-Type", "application/json");
            response.body = R"({"message": "Hello from CORS-enabled API", "data": [1, 2, 3]})";
        } else if (request.path == "/api/data" && request.method == "POST") {
            response.statusCode = 201;
            response.setHeader("Content-Type", "application/json");
            response.body = R"({"message": "Resource created", "received": ")" + request.body + R"("})";
        } else {
            response.statusCode = 404;
            response.setHeader("Content-Type", "application/json");
            response.body = R"({"error": "Not found"})";
        }

        return response;
    }

    void setConfig(const CorsConfig& cfg) {
        config = cfg;
    }
};

int main() {
    CorsConfig config;
    config.allowedOrigin = "https://example.com";
    config.allowCredentials = true;

    CorsApi api(config);

    HttpRequest preflight;
    preflight.method = "OPTIONS";
    preflight.path = "/api/data";
    preflight.headers["Origin"] = "https://example.com";
    preflight.headers["Access-Control-Request-Method"] = "POST";
    preflight.headers["Access-Control-Request-Headers"] = "Content-Type";

    HttpResponse preflightResp = api.handleRequest(preflight);
    std::cout << "=== Preflight Response ===\n" << preflightResp.toString() << "\n\n";

    HttpRequest getReq;
    getReq.method = "GET";
    getReq.path = "/api/data";
    getReq.headers["Origin"] = "https://example.com";

    HttpResponse getResp = api.handleRequest(getReq);
    std::cout << "=== GET Response ===\n" << getResp.toString() << "\n\n";

    HttpRequest postReq;
    postReq.method = "POST";
    postReq.path = "/api/data";
    postReq.headers["Origin"] = "https://example.com";
    postReq.headers["Content-Type"] = "application/json";
    postReq.body = R"({"name": "test"})";

    HttpResponse postResp = api.handleRequest(postReq);
    std::cout << "=== POST Response ===\n" << postResp.toString() << "\n";

    return 0;
}