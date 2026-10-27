#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

namespace {

constexpr std::size_t kMaximumRequestSize = 1024 * 1024;

std::string environment_value(const char* name) {
    const char* value = std::getenv(name);
    return value == nullptr ? std::string() : std::string(value);
}

std::size_t parse_content_length(const std::string& value) {
    if (value.empty()) {
        return 0;
    }

    std::size_t length = 0;
    for (unsigned char character : value) {
        if (character < '0' || character > '9') {
            return 0;
        }

        const std::size_t digit = static_cast<std::size_t>(character - '0');
        if (length > (kMaximumRequestSize - digit) / 10) {
            return kMaximumRequestSize;
        }
        length = length * 10 + digit;
    }

    return std::min(length, kMaximumRequestSize);
}

std::string read_request_data(const std::string& method,
                              const std::string& query_string,
                              std::size_t content_length,
                              std::istream& input) {
    if (method == "GET") {
        return query_string;
    }

    if (method != "POST" || content_length == 0) {
        return {};
    }

    std::string data(content_length, '\0');
    input.read(&data[0], static_cast<std::streamsize>(content_length));
    data.resize(static_cast<std::size_t>(input.gcount()));
    return data;
}

int hex_value(char character) {
    if (character >= '0' && character <= '9') {
        return character - '0';
    }
    if (character >= 'a' && character <= 'f') {
        return character - 'a' + 10;
    }
    if (character >= 'A' && character <= 'F') {
        return character - 'A' + 10;
    }
    return -1;
}

std::string url_decode(const std::string& value) {
    std::string decoded;
    decoded.reserve(value.size());

    for (std::size_t index = 0; index < value.size(); ++index) {
        if (value[index] == '+' ) {
            decoded.push_back(' ');
        } else if (value[index] == '%' && index + 2 < value.size()) {
            const int high = hex_value(value[index + 1]);
            const int low = hex_value(value[index + 2]);
            if (high >= 0 && low >= 0) {
                decoded.push_back(static_cast<char>((high << 4) | low));
                index += 2;
            } else {
                decoded.push_back(value[index]);
            }
        } else {
            decoded.push_back(value[index]);
        }
    }

    return decoded;
}

std::map<std::string, std::string> parse_form_data(const std::string& data) {
    std::map<std::string, std::string> fields;
    std::size_t start = 0;

    while (start <= data.size()) {
        const std::size_t end = data.find('&', start);
        const std::size_t segment_end =
            end == std::string::npos ? data.size() : end;
        const std::string segment = data.substr(start, segment_end - start);
        const std::size_t equals = segment.find('=');

        const std::string key = url_decode(segment.substr(0, equals));
        const std::string value =
            equals == std::string::npos
                ? std::string()
                : url_decode(segment.substr(equals + 1));

        if (!key.empty()) {
            fields[key] = value;
        }

        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
    }

    return fields;
}

std::string html_escape(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size());

    for (char character : value) {
        switch (character) {
            case '&':
                escaped += "&amp;";
                break;
            case '<':
                escaped += "&lt;";
                break;
            case '>':
                escaped += "&gt;";
                break;
            case '"':
                escaped += "&quot;";
                break;
            case '\'':
                escaped += "&#39;";
                break;
            default:
                escaped.push_back(character);
                break;
        }
    }

    return escaped;
}

std::string render_page(const std::map<std::string, std::string>& fields) {
    const auto message = fields.find("message");
    const bool has_message = message != fields.end();
    const std::string value = has_message ? message->second : std::string();

    std::ostringstream page;
    page << "<!doctype html>\n"
         << "<html lang=\"en\">\n"
         << "<head>\n"
         << "  <meta charset=\"utf-8\">\n"
         << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
         << "  <title>Message Form</title>\n"
         << "  <style>\n"
         << "    body { font-family: system-ui, sans-serif; margin: 2rem auto; "
            "max-width: 42rem; padding: 0 1rem; line-height: 1.5; }\n"
         << "    textarea { box-sizing: border-box; width: 100%; min-height: 8rem; "
            "padding: .75rem; font: inherit; }\n"
         << "    button { margin-top: .75rem; padding: .55rem 1rem; font: inherit; "
            "cursor: pointer; }\n"
         << "    .output { padding: 1rem; background: #f2f4f7; "
            "border-radius: .4rem; white-space: pre-wrap; overflow-wrap: anywhere; }\n"
         << "  </style>\n"
         << "</head>\n"
         << "<body>\n"
         << "  <main>\n"
         << "    <h1>Send a message</h1>\n"
         << "    <form method=\"post\">\n"
         << "      <label for=\"message\">Your message</label>\n"
         << "      <textarea id=\"message\" name=\"message\" "
            "placeholder=\"Type something here...\">"
         << html_escape(value)
         << "</textarea>\n"
         << "      <button type=\"submit\">Submit</button>\n"
         << "    </form>\n";

    if (has_message) {
        page << "    <section aria-labelledby=\"result-heading\">\n"
             << "      <h2 id=\"result-heading\">Your submitted message</h2>\n"
             << "      <div class=\"output\">"
             << html_escape(value)
             << "</div>\n"
             << "    </section>\n";
    }

    page << "  </main>\n"
         << "</body>\n"
         << "</html>\n";
    return page.str();
}

}  // namespace

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    const std::string method = environment_value("REQUEST_METHOD");
    const std::string query_string = environment_value("QUERY_STRING");
    const std::size_t content_length =
        parse_content_length(environment_value("CONTENT_LENGTH"));

    const std::string request_data =
        read_request_data(method, query_string, content_length, std::cin);
    const std::map<std::string, std::string> fields =
        parse_form_data(request_data);

    std::cout << "Content-Type: text/html; charset=utf-8\r\n"
              << "X-Content-Type-Options: nosniff\r\n"
              << "Content-Security-Policy: default-src 'none'; "
                 "style-src 'unsafe-inline'; form-action 'self'; base-uri 'none'\r\n"
              << "\r\n"
              << render_page(fields);

    return 0;
}