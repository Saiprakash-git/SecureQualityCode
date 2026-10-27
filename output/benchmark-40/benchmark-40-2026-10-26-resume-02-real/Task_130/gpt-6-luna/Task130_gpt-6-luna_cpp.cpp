#include <algorithm>
#include <cctype>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

struct Request {
    std::string method;
    std::string path;
    std::map<std::string, std::string> headers;
    std::string body;

    std::string header(const std::string& name) const {
        auto it = headers.find(toLower(name));
        return it == headers.end() ? "" : it->second;
    }

    static std::string toLower(std::string value) {
        std::transform(value.begin(), value.end(), value.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return value;
    }
};

struct Response {
    int status = 200;
    std::string reason = "OK";
    std::map<std::string, std::string> headers;
    std::string body;
};

struct CorsPolicy {
    std::set<std::string> allowedOrigins = {
        "http://localhost:3000",
        "https://app.example.com"
    };
    std::set<std::string> allowedMethods = {"GET", "POST", "OPTIONS"};
    std::set<std::string> allowedHeaders = {
        "authorization", "content-type", "x-requested-with"
    };
    std::vector<std::string> exposedHeaders = {"X-Request-Id"};
    bool allowCredentials = true;
    int maxAgeSeconds = 600;
};

static std::string lowerCopy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

static std::string trim(const std::string& value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

static bool isToken(const std::string& value) {
    if (value.empty()) return false;
    const std::string separators = "()<>@,;:\\\"/[]?={} \t";
    for (unsigned char c : value) {
        if (c <= 32 || c >= 127 || separators.find(static_cast<char>(c)) != std::string::npos)
            return false;
    }
    return true;
}

static std::vector<std::string> splitCommaSeparated(const std::string& value) {
    std::vector<std::string> result;
    std::istringstream stream(value);
    std::string item;
    while (std::getline(stream, item, ',')) {
        item = trim(item);
        if (!item.empty()) result.push_back(item);
    }
    return result;
}

static void addVary(Response& response, const std::string& field) {
    auto it = response.headers.find("Vary");
    if (it == response.headers.end()) {
        response.headers["Vary"] = field;
        return;
    }

    for (const auto& existing : splitCommaSeparated(it->second)) {
        if (lowerCopy(existing) == lowerCopy(field) || existing == "*") return;
    }
    it->second += ", " + field;
}

static bool originAllowed(const CorsPolicy& policy, const std::string& origin) {
    return !origin.empty() && policy.allowedOrigins.count(origin) != 0;
}

static Response makeError(int status, const std::string& reason, const std::string& message) {
    Response response;
    response.status = status;
    response.reason = reason;
    response.headers["Content-Type"] = "application/json; charset=utf-8";
    response.body = "{\"error\":\"" + message + "\"}";
    return response;
}

static Response handleApiRequest(const Request& request) {
    if (request.path != "/api/data")
        return makeError(404, "Not Found", "not_found");

    if (request.method == "GET") {
        Response response;
        response.headers["Content-Type"] = "application/json; charset=utf-8";
        response.headers["X-Request-Id"] = "demo-request";
        response.body = "{\"message\":\"Hello from the API\"}";
        return response;
    }

    if (request.method == "POST") {
        Response response;
        response.status = 201;
        response.reason = "Created";
        response.headers["Content-Type"] = "application/json; charset=utf-8";
        response.headers["X-Request-Id"] = "demo-request";
        response.body = "{\"received\":true}";
        return response;
    }

    if (request.method == "OPTIONS") {
        Response response;
        response.status = 204;
        response.reason = "No Content";
        response.headers["Allow"] = "GET, POST, OPTIONS";
        return response;
    }

    Response response = makeError(405, "Method Not Allowed", "method_not_allowed");
    response.headers["Allow"] = "GET, POST, OPTIONS";
    return response;
}

static Response handleRequest(const Request& request, const CorsPolicy& policy) {
    const std::string origin = request.header("origin");
    const std::string requestedMethod =
        trim(request.header("access-control-request-method"));
    const std::string requestedHeaders =
        request.header("access-control-request-headers");

    const bool isPreflight =
        request.method == "OPTIONS" && !origin.empty() && !requestedMethod.empty();

    if (isPreflight) {
        Response response;
        response.status = 204;
        response.reason = "No Content";
        addVary(response, "Origin");
        addVary(response, "Access-Control-Request-Method");
        addVary(response, "Access-Control-Request-Headers");

        if (request.path != "/api/data" || !originAllowed(policy, origin) ||
            !isToken(requestedMethod) ||
            policy.allowedMethods.count(requestedMethod) == 0) {
            response.status = 403;
            response.reason = "Forbidden";
            response.body = "{\"error\":\"cors_preflight_rejected\"}";
            response.headers["Content-Type"] = "application/json; charset=utf-8";
            return response;
        }

        std::vector<std::string> acceptedHeaders;
        for (const auto& header : splitCommaSeparated(requestedHeaders)) {
            if (!isToken(header) || policy.allowedHeaders.count(lowerCopy(header)) == 0) {
                response.status = 403;
                response.reason = "Forbidden";
                response.body = "{\"error\":\"cors_preflight_rejected\"}";
                response.headers["Content-Type"] = "application/json; charset=utf-8";
                return response;
            }
            acceptedHeaders.push_back(header);
        }

        response.headers["Access-Control-Allow-Origin"] = origin;
        response.headers["Access-Control-Allow-Methods"] = "GET, POST, OPTIONS";
        if (!acceptedHeaders.empty()) {
            std::string joined;
            for (const auto& header : acceptedHeaders) {
                if (!joined.empty()) joined += ", ";
                joined += header;
            }
            response.headers["Access-Control-Allow-Headers"] = joined;
        }
        if (policy.allowCredentials)
            response.headers["Access-Control-Allow-Credentials"] = "true";
        response.headers["Access-Control-Max-Age"] = std::to_string(policy.maxAgeSeconds);
        return response;
    }

    Response response = handleApiRequest(request);
    if (!origin.empty()) {
        addVary(response, "Origin");
        if (originAllowed(policy, origin)) {
            response.headers["Access-Control-Allow-Origin"] = origin;
            if (policy.allowCredentials)
                response.headers["Access-Control-Allow-Credentials"] = "true";

            if (!policy.exposedHeaders.empty()) {
                std::string exposed;
                for (const auto& header : policy.exposedHeaders) {
                    if (!exposed.empty()) exposed += ", ";
                    exposed += header;
                }
                response.headers["Access-Control-Expose-Headers"] = exposed;
            }
        }
    }
    return response;
}

static bool parseHttpRequest(const std::string& input, Request& request) {
    const std::size_t headerEnd = input.find("\r\n\r\n");
    const std::size_t separatorLength = 4;
    const std::size_t alternateHeaderEnd = input.find("\n\n");
    const std::size_t end = headerEnd != std::string::npos ? headerEnd : alternateHeaderEnd;
    if (end == std::string::npos || end > 65536) return false;

    const std::size_t bodyStart = end + (headerEnd != std::string::npos ? separatorLength : 2);
    std::string headerBlock = input.substr(0, end);
    std::replace(headerBlock.begin(), headerBlock.end(), '\r', '\n');

    std::istringstream lines(headerBlock);
    std::string line;
    if (!std::getline(lines, line)) return false;
    std::istringstream requestLine(line);
    std::string target;
    std::string version;
    if (!(requestLine >> request.method >> target >> version)) return false;
    if (version.rfind("HTTP/", 0) != 0) return false;

    const std::size_t query = target.find('?');
    request.path = target.substr(0, query);

    while (std::getline(lines, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        const std::size_t colon = line.find(':');
        if (colon == std::string::npos) return false;
        const std::string name = trim(line.substr(0, colon));
        if (!isToken(name)) return false;
        const std::string key = lowerCopy(name);
        const std::string value = trim(line.substr(colon + 1));
        auto it = request.headers.find(key);
        if (it == request.headers.end()) request.headers[key] = value;
        else it->second += ", " + value;
    }

    std::size_t contentLength = 0;
    const auto contentLengthHeader = request.headers.find("content-length");
    if (contentLengthHeader != request.headers.end()) {
        try {
            std::size_t parsed = 0;
            contentLength = std::stoull(contentLengthHeader->second, &parsed);
            if (parsed != contentLengthHeader->second.size() || contentLength > 1'000'000)
                return false;
        } catch (...) {
            return false;
        }
    }

    if (input.size() < bodyStart + contentLength) return false;
    request.body = input.substr(bodyStart, contentLength);
    return true;
}

static std::string serializeResponse(const Response& response) {
    std::ostringstream output;
    output << "HTTP/1.1 " << response.status << ' ' << response.reason << "\r\n";
    for (const auto& header : response.headers)
        output << header.first << ": " << header.second << "\r\n";
    output << "Content-Length: " << response.body.size() << "\r\n";
    output << "Connection: close\r\n\r\n";
    output << response.body;
    return output.str();
}

int main() {
    std::ostringstream inputBuffer;
    inputBuffer << std::cin.rdbuf();

    Request request;
    if (!parseHttpRequest(inputBuffer.str(), request)) {
        Response response = makeError(400, "Bad Request", "invalid_http_request");
        std::cout << serializeResponse(response);
        return 0;
    }

    const CorsPolicy policy;
    std::cout << serializeResponse(handleRequest(request, policy));
    return 0;
}