#include <curl/curl.h>

#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>

static size_t writeCallback(char* data, size_t size, size_t count, void* userData) {
    const size_t bytes = size * count;
    static_cast<std::string*>(userData)->append(data, bytes);
    return bytes;
}

static std::string fetchPage(const std::string& url) {
    std::string page;

    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        return page;
    }

    CURL* curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &page);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");
        curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTPS);
        curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTPS);

        if (curl_easy_perform(curl) != CURLE_OK) {
            page.clear();
        }
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    return page;
}

static bool startsWithIgnoreCase(const std::string& text, size_t position,
                                 const std::string& prefix) {
    if (position + prefix.size() > text.size()) {
        return false;
    }

    for (size_t i = 0; i < prefix.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(text[position + i])) !=
            std::tolower(static_cast<unsigned char>(prefix[i]))) {
            return false;
        }
    }
    return true;
}

static void appendUtf8(std::string& output, uint32_t codepoint) {
    if (codepoint <= 0x7F) {
        output.push_back(static_cast<char>(codepoint));
    } else if (codepoint <= 0x7FF) {
        output.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
        output.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else if (codepoint <= 0xFFFF && !(codepoint >= 0xD800 && codepoint <= 0xDFFF)) {
        output.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
        output.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        output.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else if (codepoint <= 0x10FFFF) {
        output.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
        output.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
        output.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        output.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    }
}

static std::string decodeHtmlEntities(const std::string& text) {
    static const std::unordered_map<std::string, std::string> entities = {
        {"amp", "&"}, {"lt", "<"}, {"gt", ">"}, {"quot", "\""},
        {"apos", "'"}, {"nbsp", "\xC2\xA0"}
    };

    std::string output;
    output.reserve(text.size());

    for (size_t i = 0; i < text.size();) {
        if (text[i] != '&') {
            output.push_back(text[i++]);
            continue;
        }

        const size_t semicolon = text.find(';', i + 1);
        if (semicolon == std::string::npos || semicolon - i > 12) {
            output.push_back(text[i++]);
            continue;
        }

        const std::string entity = text.substr(i + 1, semicolon - i - 1);
        const auto named = entities.find(entity);
        if (named != entities.end()) {
            output += named->second;
            i = semicolon + 1;
            continue;
        }

        if (entity.size() > 1 && entity[0] == '#') {
            char* end = nullptr;
            const bool hexadecimal = entity.size() > 2 &&
                (entity[1] == 'x' || entity[1] == 'X');
            const char* digits = entity.c_str() + (hexadecimal ? 2 : 1);
            const unsigned long value = std::strtoul(digits, &end, hexadecimal ? 16 : 10);

            if (end != digits && *end == '\0' && value <= 0x10FFFF &&
                !(value >= 0xD800 && value <= 0xDFFF)) {
                appendUtf8(output, static_cast<uint32_t>(value));
                i = semicolon + 1;
                continue;
            }
        }

        output.append(text, i, semicolon - i + 1);
        i = semicolon + 1;
    }

    return output;
}

static std::string extractTitle(const std::string& html) {
    size_t opening = std::string::npos;

    for (size_t i = 0; i < html.size(); ++i) {
        if (html[i] == '<' && startsWithIgnoreCase(html, i + 1, "title")) {
            const size_t boundary = i + 6;
            if (boundary < html.size() &&
                (std::isspace(static_cast<unsigned char>(html[boundary])) ||
                 html[boundary] == '>')) {
                opening = i;
                break;
            }
        }
    }

    if (opening == std::string::npos) {
        return "";
    }

    const size_t contentStart = html.find('>', opening);
    if (contentStart == std::string::npos) {
        return "";
    }

    size_t closing = std::string::npos;
    for (size_t i = contentStart + 1; i < html.size(); ++i) {
        if (html[i] == '<' && startsWithIgnoreCase(html, i + 1, "/title")) {
            const size_t boundary = i + 7;
            if (boundary < html.size() &&
                (std::isspace(static_cast<unsigned char>(html[boundary])) ||
                 html[boundary] == '>')) {
                closing = i;
                break;
            }
        }
    }

    if (closing == std::string::npos) {
        return "";
    }

    std::string title = decodeHtmlEntities(
        html.substr(contentStart + 1, closing - contentStart - 1));

    size_t first = 0;
    while (first < title.size() &&
           std::isspace(static_cast<unsigned char>(title[first]))) {
        ++first;
    }

    size_t last = title.size();
    while (last > first &&
           std::isspace(static_cast<unsigned char>(title[last - 1]))) {
        --last;
    }

    return title.substr(first, last - first);
}

static std::string getPageTitle(const std::string& url) {
    return extractTitle(fetchPage(url));
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        return 1;
    }

    std::cout << getPageTitle(argv[1]);
    return 0;
}