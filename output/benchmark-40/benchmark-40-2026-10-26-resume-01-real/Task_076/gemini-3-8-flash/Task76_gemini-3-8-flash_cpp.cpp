#include <iostream>
#include <string>
#include <regex>
#include <curl/curl.h>

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    std::string* str = static_cast<std::string*>(userp);
    str->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

std::string fetchUrl(const std::string& url) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        return "";
    }

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (compatible; TitleExtractor/1.0)");

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        return "";
    }

    return response;
}

std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) {
        return "";
    }
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

std::string extractTitle(const std::string& html) {
    std::regex title_regex("<title[^>]*>([\\s\\S]*?)</title>", std::regex::icase);
    std::smatch match;
    if (std::regex_search(html, match, title_regex) && match.size() > 1) {
        return trim(match[1].str());
    }
    return "";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <HTTPS_URL>" << std::endl;
        return 1;
    }

    std::string url = argv[1];

    curl_global_init(CURL_GLOBAL_DEFAULT);

    std::string htmlContent = fetchUrl(url);
    if (htmlContent.empty()) {
        std::cerr << "Failed to retrieve content or content is empty." << std::endl;
        curl_global_cleanup();
        return 1;
    }

    std::string title = extractTitle(htmlContent);
    if (!title.empty()) {
        std::cout << title << std::endl;
    } else {
        std::cout << "No title found." << std::endl;
    }

    curl_global_cleanup();
    return 0;
}