#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <sstream>
#include <algorithm>
#include <cctype>

struct HttpRequest {
    std::string method;
    std::string path;
    std::map<std::string, std::string> headers;
    std::string body;

    std::string getHeader(const std::string& name) const {
        std::string lower_name = name;
        std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
        auto it = headers.find(lower_name);
        if (it != headers.end()) {
            return it->second;
        }
        return "";
    }

    void setHeader(const std::string& name, const std::string& value) {
        std::string lower_name = name;
        std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
        headers[lower_name] = value;
    }
};

struct HttpResponse {
    int status_code = 200;
    std::string status_text = "OK";
    std::map<std::string, std::string> headers;
    std::string body;

    void setHeader(const std::string& name, const std::string& value) {
        headers[name] = value;
    }

    std::string toString() const {
        std::ostringstream oss;
        oss << "HTTP/1.1 " << status_code << " " << status_text << "\r\n";
        for (const auto& pair : headers) {
            oss << pair.first << ": " << pair.second << "\r\n";
        }
        oss << "\r\n" << body;
        return oss.str();
    }
};

struct CorsConfig {
    std::set<std::string> allowed_origins;
    std::set<std::string> allowed_methods;
    std::set<std::string> allowed_headers;
    std::set<std::string> exposed_headers;
    bool allow_credentials = false;
    int max_age_seconds = 86400;
    bool allow_all_origins = false;
};

class CorsHandler {
private:
    CorsConfig config;

    static std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }

    static std::vector<std::string> split(const std::string& str, char delim) {
        std::vector<std::string> tokens;
        std::stringstream ss(str);
        std::string token;
        while (std::getline(ss, token, delim)) {
            std::string t = trim(token);
            if (!t.empty()) {
                tokens.push_back(t);
            }
        }
        return tokens;
    }

    static std::string join(const std::set<std::string>& elements, const std::string& delimiter) {
        std::string result;
        for (auto it = elements.begin(); it != elements.end(); ++it) {
            if (it != elements.begin()) {
                result += delimiter;
            }
            result += *it;
        }
        return result;
    }

    bool isOriginAllowed(const std::string& origin) const {
        if (origin.empty()) return false;
        if (config.allow_all_origins) return true;
        return config.allowed_origins.find(origin) != config.allowed_origins.end();
    }

    bool areHeadersAllowed(const std::string& req_headers) const {
        if (req_headers.empty()) return true;
        std::vector<std::string> headers = split(req_headers, ',');
        for (const auto& h : headers) {
            std::string lower_h = h;
            std::transform(lower_h.begin(), lower_h.end(), lower_h.begin(), ::tolower);
            
            bool found = false;
            for (const auto& allowed : config.allowed_headers) {
                std::string lower_allowed = allowed;
                std::transform(lower_allowed.begin(), lower_allowed.end(), lower_allowed.begin(), ::tolower);
                if (lower_allowed == lower_h || lower_allowed == "*") {
                    found = true;
                    break;
                }
            }
            if (!found) return false;
        }
        return true;
    }

    bool isMethodAllowed(const std::string& method) const {
        return config.allowed_methods.find(method) != config.allowed_methods.end();
    }

public:
    explicit CorsHandler(const CorsConfig& cfg) : config(cfg) {}

    bool isCorsRequest(const HttpRequest& req) const {
        return !req.getHeader("origin").empty();
    }

    bool isPreflightRequest(const HttpRequest& req) const {
        return req.method == "OPTIONS" && 
               !req.getHeader("origin").empty() && 
               !req.getHeader("access-control-request-method").empty();
    }

    HttpResponse handlePreflight(const HttpRequest& req) const {
        HttpResponse response;
        std::string origin = req.getHeader("origin");

        if (!isOriginAllowed(origin)) {
            response.status_code = 403;
            response.status_text = "Forbidden";
            response.body = "CORS preflight channel rejected: Origin not allowed.";
            return response;
        }

        std::string request_method = req.getHeader("access-control-request-method");
        if (!isMethodAllowed(request_method)) {
            response.status_code = 403;
            response.status_text = "Forbidden";
            response.body = "CORS preflight channel rejected: Method not allowed.";
            return response;
        }

        std::string request_headers = req.getHeader("access-control-request-headers");
        if (!areHeadersAllowed(request_headers)) {
            response.status_code = 403;
            response.status_text = "Forbidden";
            response.body = "CORS preflight channel rejected: Headers not allowed.";
            return response;
        }

        response.status_code = 204;
        response.status_text = "No Content";

        if (config.allow_credentials) {
            response.setHeader("Access-Control-Allow-Origin", origin);
            response.setHeader("Access-Control-Allow-Credentials", "true");
        } else {
            response.setHeader("Access-Control-Allow-Origin", config.allow_all_origins ? "*" : origin);
        }

        response.setHeader("Access-Control-Allow-Methods", join(config.allowed_methods, ", "));
        
        if (!config.allowed_headers.empty()) {
            response.setHeader("Access-Control-Allow-Headers", join(config.allowed_headers, ", "));
        }
        
        response.setHeader("Access-Control-Max-Age", std::to_string(config.max_age_seconds));
        response.setHeader("Vary", "Origin, Access-Control-Request-Method, Access-Control-Request-Headers");
        return response;
    }

    void applyActualRequestHeaders(const HttpRequest& req, HttpResponse& res) const {
        if (!isCorsRequest(req)) {
            return;
        }

        std::string origin = req.getHeader("origin");
        if (!isOriginAllowed(origin)) {
            return;
        }

        if (config.allow_credentials) {
            res.setHeader("Access-Control-Allow-Origin", origin);
            res.setHeader("Access-Control-Allow-Credentials", "true");
        } else {
            res.setHeader("Access-Control-Allow-Origin", config.allow_all_origins ? "*" : origin);
        }

        if (!config.exposed_headers.empty()) {
            res.setHeader("Access-Control-Expose-Headers", join(config.exposed_headers, ", "));
        }

        res.setHeader("Vary", "Origin");
    }
};

class UserApi {
private:
    CorsHandler cors_handler;

    HttpResponse handleGetUsers() {
        HttpResponse res;
        res.status_code = 200;
        res.status_text = "OK";
        res.setHeader("Content-Type", "application/json");
        res.setHeader("X-Server-Execution-Time", "12ms");
        res.body = R"([{"id": 1, "name": "Alice"}, {"id": 2, "name": "Bob"}])";
        return res;
    }

    HttpResponse handleCreateUser(const std::string& body) {
        HttpResponse res;
        res.status_code = 201;
        res.status_text = "Created";
        res.setHeader("Content-Type", "application/json");
        res.setHeader("X-Server-Execution-Time", "25ms");
        res.body = R"({"status": "success", "received": )" + (body.empty() ? "{}" : body) + "}";
        return res;
    }

public:
    explicit UserApi(const CorsConfig& config) : cors_handler(config) {}

    HttpResponse processRequest(const HttpRequest& req) {
        if (cors_handler.isPreflightRequest(req)) {
            return cors_handler.handlePreflight(req);
        }

        HttpResponse response;
        if (req.path == "/api/users") {
            if (req.method == "GET") {
                response = handleGetUsers();
            } else if (req.method == "POST") {
                response = handleCreateUser(req.body);
            } else {
                response.status_code = 405;
                response.status_text = "Method Not Allowed";
                response.body = R"({"error": "Method Not Allowed"})";
            }
        } else {
            response.status_code = 404;
            response.status_text = "Not Found";
            response.body = R"({"error": "Endpoint Not Found"})";
        }

        cors_handler.applyActualRequestHeaders(req, response);
        return response;
    }
};

void runScenario(UserApi& api, const std::string& scenario_name, const HttpRequest& req) {
    std::cout << "========================================================\n";
    std::cout << "Scenario: " << scenario_name << "\n";
    std::cout << "Request:  " << req.method << " " << req.path << "\n";
    for (const auto& h : req.headers) {
        std::cout << "  " << h.first << ": " << h.second << "\n";
    }
    std::cout << "\nResponse Output:\n";
    HttpResponse res = api.processRequest(req);
    std::cout << res.toString() << "\n\n";
}

int main() {
    CorsConfig cors_config;
    cors_config.allowed_origins = {"https://portal.example.com", "https://admin.example.com"};
    cors_config.allowed_methods = {"GET", "POST", "OPTIONS"};
    cors_config.allowed_headers = {"Authorization", "Content-Type", "X-Client-Trace-Id"};
    cors_config.exposed_headers = {"X-Server-Execution-Time"};