#include <iostream>
#include <string>
#include <map>
#include <cstdlib>
#include <sstream>

std::string urlDecode(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.length(); i++) {
        if (str[i] == '%') {
            if (i + 2 < str.length()) {
                std::string hex = str.substr(i + 1, 2);
                char* end;
                long val = std::strtol(hex.c_str(), &end, 16);
                if (*end == '\0') {
                    result += static_cast<char>(val);
                    i += 2;
                    continue;
                }
            }
            result += str[i];
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }
    return result;
}

std::map<std::string, std::string> parseFormData(const std::string& data) {
    std::map<std::string, std::string> result;
    std::stringstream ss(data);
    std::string pair;

    while (std::getline(ss, pair, '&')) {
        size_t pos = pair.find('=');
        if (pos != std::string::npos) {
            std::string key = urlDecode(pair.substr(0, pos));
            std::string value = urlDecode(pair.substr(pos + 1));
            result[key] = value;
        }
    }
    return result;
}

std::string htmlEscape(const std::string& str) {
    std::string result;
    for (char c : str) {
        switch (c) {
            case '&': result += "&amp;"; break;
            case '<': result += "&lt;"; break;
            case '>': result += "&gt;"; break;
            case '"': result += "&quot;"; break;
            case '\'': result += "&#39;"; break;
            default: result += c;
        }
    }
    return result;
}

int main() {
    std::cout << "Content-Type: text/html\r\n\r\n";

    std::string method = std::getenv("REQUEST_METHOD") ? std::getenv("REQUEST_METHOD") : "GET";
    std::map<std::string, std::string> formData;

    if (method == "POST") {
        std::string contentLengthStr = std::getenv("CONTENT_LENGTH") ? std::getenv("CONTENT_LENGTH") : "0";
        int contentLength = std::atoi(contentLengthStr.c_str());
        if (contentLength > 0) {
            std::string postData(contentLength, '\0');
            std::cin.read(&postData[0], contentLength);
            formData = parseFormData(postData);
        }
    } else if (method == "GET") {
        std::string queryString = std::getenv("QUERY_STRING") ? std::getenv("QUERY_STRING") : "";
        formData = parseFormData(queryString);
    }

    std::cout << "<!DOCTYPE html>\n";
    std::cout << "<html>\n";
    std::cout << "<head>\n";
    std::cout << "<title>Form Echo</title>\n";
    std::cout << "<style>\n";
    std::cout << "body { font-family: Arial, sans-serif; max-width: 600px; margin: 50px auto; padding: 20px; }\n";
    std::cout << "form { background: #f0f0f0; padding: 20px; border-radius: 5px; }\n";
    std::cout << "input, textarea { width: 100%; padding: 8px; margin: 5px 0; box-sizing: border-box; }\n";
    std::cout << "button { background: #007bff; color: white; padding: 10px 20px; border: none; cursor: pointer; }\n";
    std::cout << "button:hover { background: #0056b3; }\n";
    std::cout << ".result { background: #e9ecef; padding: 15px; margin-top: 20px; border-radius: 5px; }\n";
    std::cout << "</style>\n";
    std::cout << "</head>\n";
    std::cout << "<body>\n";
    std::cout << "<h1>Form Echo Application</h1>\n";

    if (!formData.empty()) {
        std::cout << "<div class='result'>\n";
        std::cout << "<h2>Submitted Data:</h2>\n";
        std::cout << "<ul>\n";
        for (const auto& pair : formData) {
            std::cout << "<li><strong>" << htmlEscape(pair.first) << ":</strong> " << htmlEscape(pair.second) << "</li>\n";
        }
        std::cout << "</ul>\n";
        std::cout << "</div>\n";
    }

    std::cout << "<form method='POST' action=''>\n";
    std::cout << "<label>Name: <input type='text' name='name' value='" << (formData.count("name") ? htmlEscape(formData["name"]) : "") << "'></label><br>\n";
    std::cout << "<label>Email: <input type='email' name='email' value='" << (formData.count("email") ? htmlEscape(formData["email"]) : "") << "'></label><br>\n";
    std::cout << "<label>Message: <textarea name='message'>" << (formData.count("message") ? htmlEscape(formData["message"]) : "") << "</textarea></label><br>\n";
    std::cout << "<button type='submit'>Submit</button>\n";
    std::cout << "</form>\n";

    std::cout << "</body>\n";
    std::cout << "</html>\n";

    return 0;
}