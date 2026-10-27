```cpp
#include <curl/curl.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <string>

static size_t writeCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* buffer = static_cast<std::string*>(userdata);
    size_t total = size * nmemb;
    buffer->append(ptr, total);
    return total;
}

std::string fetchUrlContent(const std::string& url, std::string& errorMsg) {
    std::string body;
    errorMsg.clear();

    CURL* curl = curl_easy_init();
    if (!curl) {
        errorMsg = "Failed to initialize libcurl.";
        return "";
    }

    char errbuf[CURL_ERROR_SIZE] = {0};
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (title-extractor)");
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        errorMsg = std::string("HTTP request failed: ") + (errbuf[0] ? errbuf : curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        return "";
    }

    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_easy_cleanup(curl);

    if (httpCode >= 400) {
        errorMsg = "HTTP error status: " + std::to_string(httpCode);
        return "";
    }

    return body;
}

static std::string decodeHtmlEntities(const std::string& in) {
    static const std::pair<const char*, const char*> entities[] = {
        {"&amp;", "&"}, {"&lt;", "<"}, {"&gt;", ">"},
        {"&quot;", "\""}, {"&#39;", "'"}, {"&#x27;", "'"},
        {"&apos;", "'"}, {"&nbsp;", " "}
    };
    std::string out = in;
    for (const auto& e : entities) {
        std::string from = e.first;
        std::string to = e.second;
        size_t pos = 0;
        while ((pos = out.find(from, pos)) != std::string::npos) {
            out.replace(pos, from.size(), to);
            pos += to.size();
        }
    }
    return out;
}

std::string extractPageTitle(const std::string& html) {
    std::string lower = html;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    size_t start = lower.find("<title");
    if (start == std::string::npos) {
        return "";
    }

    size_t openEnd = lower.find('>', start);
    if (openEnd == std::string::npos) {
        return "";
    }

    size_t close = lower.find("</title", openEnd + 1);
    if (close == std::string::npos) {
        return "";
    }

    std::string title = html.substr(openEnd + 1, close - (openEnd + 1));

    // Collapse whitespace and trim
    std::string collapsed;
    bool lastSpace = false;
    for (unsigned char c : title) {
        if (std::isspace(c)) {
            if (!lastSpace && !collapsed.empty()) {
                collapsed += ' ';
            }
            lastSpace = true;
        } else {
            collapsed += static_cast<char>(c);
            lastSpace = false;
        }
    }
    while (!collapsed.empty() && collapsed.back() == ' ') {
        collapsed.pop_back();
    }

    return decodeHtmlEntities(collapsed);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <https-url>" << std::endl;
        return 1;
    }

    std::string url = argv[1];
    if (url.rfind("https://", 0) != 0) {
        std::cerr << "Error: URL must start with https://" << std::endl;
        return 1;
    }

    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        std::cerr << "Error: Failed to initialize curl globally." << std::endl;
        return 1;
    }

    std::string errorMsg;
    std::string html = fetchUrlContent(url, errorMsg);

    if (!errorMsg.empty()) {
        std::cerr << "Error: " << errorMsg << std::endl;
        curl_global_cleanup();
        return 1;
    }

    std::string title = extractPageTitle(html);
    if (title.empty()) {
        std::cerr << "Error: No page title found." << std::endl;
        curl_global_cleanup();
        return 1;
    }

    std::cout << title << std::endl;

    curl_global_cleanup();
    return 0;
}
```