#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <iostream>
#include <map>
#include <string>

struct HttpRequest {
    std::string method;
    std::string path;
    std::map<std::string, std::string> headers;
    std::string body;
};

static std::string toLower(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

static int hexValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

std::string urlDecode(const std::string& input) {
    std::string out;
    out.reserve(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];
        if (c == '+') {
            out += ' ';
        } else if (c == '%' && i + 2 < input.size() + 0 && i + 2 <= input.size() - 1 &&
                   hexValue(input[i + 1]) >= 0 && hexValue(input[i + 2]) >= 0) {
            out += static_cast<char>(hexValue(input[i + 1]) * 16 + hexValue(input[i + 2]));
            i += 2;
        } else {
            out += c;
        }
    }
    return out;
}

std::map<std::string, std::string> parseFormData(const std::string& body) {
    std::map<std::string, std::string> fields;
    size_t start = 0;
    while (start <= body.size()) {
        size_t amp = body.find('&', start);
        if (amp == std::string::npos) amp = body.size();
        std::string pair = body.substr(start, amp - start);
        if (!pair.empty()) {
            size_t eq = pair.find('=');
            std::string key, value;
            if (eq == std::string::npos) {
                key = urlDecode(pair);
            } else {
                key = urlDecode(pair.substr(0, eq));
                value = urlDecode(pair.substr(eq + 1));
            }
            if (!key.empty()) fields[key] = value;
        }
        start = amp + 1;
    }
    return fields;
}

std::string htmlEscape(const std::string& input) {
    std::string out;
    out.reserve(input.size());
    for (char c : input) {
        switch (c) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            case '\'': out += "&#39;"; break;
            default: out += c; break;
        }
    }
    return out;
}

std::string renderPage(const std::string& title, const std::string& bodyContent) {
    std::string html;
    html += "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n";
    html += "<meta charset=\"UTF-8\">\n";
    html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    html += "<title>" + htmlEscape(title) + "</title>\n";
    html += "<style>\n"
            "body { font-family: Arial, sans-serif; background: #f4f6f8; margin: 0; padding: 20px; }\n"
            ".card { max-width: 600px; margin: 40px auto; background: #fff; padding: 30px;"
            " border-radius: 8px; box-shadow: 0 2px 8px rgba(0,0,0,0.1); }\n"
            "h1 { margin-top: 0; color: #333; }\n"
            "label { display: block; margin-top: 15px; font-weight: bold; color: #555; }\n"
            "input[type=text], input[type=email], textarea {"
            " width: 100%; padding: 8px; margin-top: 5px; box-sizing: border-box;"
            " border: 1px solid #ccc; border-radius: 4px; font-size: 14px; }\n"
            "textarea { height: 120px; resize: vertical; }\n"
            "button { margin-top: 20px; padding: 10px 20px; background: #0078d4; color: #fff;"
            " border: none; border-radius: 4px; font-size: 15px; cursor: pointer; }\n"
            "button:hover { background: #005a9e; }\n"
            ".result { margin-top: 10px; padding: 12px; background: #eef6ff;"
            " border-left: 4px solid #0078d4; border-radius: 4px; white-space: pre-wrap; word-wrap: break-word; }\n"
            ".field-name { font-weight: bold; color: #333; }\n"
            "a { color: #0078d4; }\n"
            "</style>\n</head>\n<body>\n<div class=\"card\">\n";
    html += bodyContent;
    html += "\n</div>\n</body>\n</html>\n";
    return html;
}

std::string renderFormPage() {
    std::string body;
    body += "<h1>Submit Your Information</h1>\n";
    body += "<form method=\"POST\" action=\"/submit\">\n";
    body += "<label for=\"name\">Full Name</label>\n";
    body += "<input type=\"text\" id=\"name\" name=\"name\" required>\n";
    body += "<label for=\"email\">Email</label>\n";
    body += "<input type=\"email\" id=\"email\" name=\"email\" required>\n";
    body += "<label for=\"message\">Message</label>\n";
    body += "<textarea id=\"message\" name=\"message\" required></textarea>\n";
    body += "<button type=\"submit\">Submit</button>\n";
    body += "</form>\n";
    return renderPage("Input Form", body);
}

std::string renderResultPage(const std::map<std::string, std::string>& fields) {
    std::string body;
    body += "<h1>Submitted Information</h1>\n";
    if (fields.empty()) {
        body += "<p>No data was received.</p>\n";
    }
    for (const auto& kv : fields) {
        body += "<label class=\"field-name\">" + htmlEscape(kv.first) + "</label>\n";
        body += "<div class=\"result\">" + htmlEscape(kv.second) + "</div>\n";
    }
    body += "<p><a href=\"/\">&larr; Submit another response</a></p>\n";
    return renderPage("Submitted Information", body);
}

std::string renderNotFoundPage() {
    return renderPage("Not Found",
                      "<h1>404 Not Found</h1><p>The requested page does not exist.</p>"
                      "<p><a href=\"/\">Go to form</a></p>");
}

std::string buildHttpResponse(int statusCode, const std::string& statusText,
                              const std::string& contentType, const std::string& body) {
    std::string resp;
    resp += "HTTP/1.1 " + std::to_string(statusCode) + " " + statusText + "\r\n";
    resp += "Content-Type: " + contentType + "\r\n";
    resp += "Content-Length: " + std::to_string(body.size()) + "\r\n";
    resp += "Connection: close\r\n\r\n";
    resp += body;
    return resp;
}

bool parseHttpRequest(const std::string& raw, HttpRequest& req) {
    size_t headerEnd = raw.find("\r\n\r\n");
    if (headerEnd == std::string::npos) return false;

    std::string head = raw.substr(0, headerEnd);
    size_t lineEnd = head.find("\r\n");
    std::string requestLine = (lineEnd == std::string::npos) ? head : head.substr(0, lineEnd);

    size_t sp1 = requestLine.find(' ');
    size_t sp2 = (sp1 == std::string::npos) ? std::string::npos : requestLine.find(' ', sp1 + 1);
    if (sp1 == std::string::npos || sp2 == std::string::npos) return false;

    req.method = requestLine.substr(0, sp1);
    req.path = requestLine.substr(sp1 + 1, sp2 - sp1 - 1);
    size_t q = req.path.find('?');
    if (q != std::string::npos) req.path = req.path.substr(0, q);

    size_t pos = (lineEnd == std::string::npos) ? head.size() : lineEnd + 2;
    while (pos < head.size()) {
        size_t next = head.find("\r\n", pos);
        if (next == std::string::npos) next = head.size();
        std::string line = head.substr(pos, next - pos);
        size_t colon = line.find(':');
        if (colon != std::string::npos) {
            std::string name = toLower(line.substr(0, colon));
            std::string value = line.substr(colon + 1);
            size_t first = value.find_first_not_of(" \t");
            value = (first == std::string::npos) ? "" : value.substr(first);
            req.headers[name] = value;
        }
        pos = next + 2;
    }

    req.body = raw.substr(headerEnd + 4);
    return true;
}

std::string handleRequest(const HttpRequest& req) {
    if (req.method == "GET" && (req.path == "/" || req.path == "/form")) {
        return buildHttpResponse(200, "OK", "text/html; charset=utf-8", renderFormPage());
    }
    if (req.method == "POST" && req.path == "/submit") {
        auto fields = parseFormData(req.body);
        return buildHttpResponse(200, "OK", "text/html; charset=utf-8", renderResultPage(fields));
    }
    return buildHttpResponse(404, "Not Found", "text/html; char