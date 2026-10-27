#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

std::string htmlEscape(const std::string& input) {
    std::string escaped;
    escaped.reserve(input.size());
    for (char c : input) {
        switch (c) {
            case '&':  escaped.append("&amp;");  break;
            case '<':  escaped.append("&lt;");   break;
            case '>':  escaped.append("&gt;");   break;
            case '"':  escaped.append("&quot;"); break;
            case '\'': escaped.append("&#39;");  break;
            default:   escaped.push_back(c);     break;
        }
    }
    return escaped;
}

std::string urlDecode(const std::string& input) {
    std::string decoded;
    decoded.reserve(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '+') {
            decoded.push_back(' ');
        } else if (input[i] == '%' && i + 2 < input.size()) {
            std::string hexStr = input.substr(i + 1, 2);
            char ch = static_cast<char>(std::strtol(hexStr.c_str(), nullptr, 16));
            decoded.push_back(ch);
            i += 2;
        } else {
            decoded.push_back(input[i]);
        }
    }
    return decoded;
}

std::string extractFormField(const std::string& body, const std::string& key) {
    std::string token = key + "=";
    size_t start = body.find(token);
    if (start == std::string::npos) {
        return "";
    }
    start += token.length();
    size_t end = body.find('&', start);
    if (end == std::string::npos) {
        end = body.length();
    }
    return urlDecode(body.substr(start, end - start));
}

std::string generateHtml(const std::string& userInput) {
    std::ostringstream oss;
    oss << "<!DOCTYPE html>\n"
        << "<html lang=\"en\">\n"
        << "<head>\n"
        << "    <meta charset=\"UTF-8\">\n"
        << "    <title>C++ Web Form</title>\n"
        << "    <style>\n"
        << "        body { font-family: Arial, sans-serif; margin: 40px; background-color: #f4f4f9; }\n"
        << "        .container { background: #fff; padding: 20px; border-radius: 8px; max-width: 500px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }\n"
        << "        input[type=\"text\"] { width: 100%; padding: 8px; margin: 8px 0; box-sizing: border-box; }\n"
        << "        input[type=\"submit\"] { background-color: #007bff; color: white; border: none; padding: 10px 15px; border-radius: 4px; cursor: pointer; }\n"
        << "        .result { margin-top: 20px; padding: 10px; background-color: #e7f3fe; border-left: 6px solid #2196F3; }\n"
        << "    </style>\n"
        << "</head>\n"
        << "<body>\n"
        << "    <div class=\"container\">\n"
        << "        <h2>Input Form</h2>\n"
        << "        <form method=\"POST\" action=\"/\">\n"
        << "            <label for=\"message\">Enter a message:</label>\n"
        << "            <input type=\"text\" id=\"message\" name=\"message\" required>\n"
        << "            <input type=\"submit\" value=\"Submit\">\n"
        << "        </form>\n";

    if (!userInput.empty()) {
        oss << "        <div class=\"result\">\n"
            << "            <strong>You submitted:</strong> " << htmlEscape(userInput) << "\n"
            << "        </div>\n";
    }

    oss << "    </div>\n"
        << "</body>\n"
        << "</html>\n";

    return oss.str();
}

std::string handleHttpRequest(const std::string& request) {
    std::string method;
    std::string path;
    std::istringstream stream(request);
    stream >> method >> path;

    std::string responseBody;

    if (method == "GET") {
        responseBody = generateHtml("");
    } else if (method == "POST") {
        size_t headerEnd = request.find("\r\n\r\n");
        std::string body;
        if (headerEnd != std::string::npos) {
            body = request.substr(headerEnd + 4);
        }
        std::string message = extractFormField(body, "message");
        responseBody = generateHtml(message);
    } else {
        responseBody = "<html><body><h1>405 Method Not Allowed</h1></body></html>";
        std::ostringstream response;
        response << "HTTP/1.1 405 Method Not Allowed\r\n"
                 << "Content-Type: text/html\r\n"
                 << "Content-Length: " << responseBody.size() << "\r\n"
                 << "Connection: close\r\n\r\n"
                 << responseBody;
        return response.str();
    }

    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n"
             << "Content-Type: text/html; charset=UTF-8\r\n"
             << "Content-Length: " << responseBody.size() << "\r\n"
             << "Connection: close\r\n\r\n"
             << responseBody;
    return response.str();
}

int startServer(int port) {
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) {
        std::cerr << "Failed to create socket\n";
        return 1;
    }

    int opt = 1;
    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Failed to set socket options\n";
        close(serverFd);
        return 1;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(serverFd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed\n";
        close(serverFd);
        return 1;
    }

    if (listen(serverFd, 10) < 0) {
        std::cerr << "Listen failed\n";
        close(serverFd);
        return 1;
    }

    std::cout << "Web application running at http://localhost:" << port << "/\n";

    while (true) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int clientFd = accept(serverFd, (struct sockaddr*)&clientAddr, &clientLen);
        if (clientFd < 0) {
            std::cerr << "Accept failed\n";
            continue;
        }

        std::vector<char> buffer(4096);
        ssize_t bytesRead = read(clientFd, buffer.data(), buffer.size() - 1);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            std::string request(buffer.data(), bytesRead);
            std::string response = handleHttpRequest(request);
            write(clientFd, response.c_str(), response.size());
        }

        close(clientFd);
    }

    close(serverFd);
    return 0;
}

int main() {
    int port = 8080;
    return startServer(port);
}