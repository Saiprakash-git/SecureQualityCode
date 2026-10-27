#include <iostream>
#include <string>
#include <curl/curl.h>
#include <algorithm>
#include <cctype>

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t total_size = size * nmemb;
    output->append((char*)contents, total_size);
    return total_size;
}

std::string fetchUrl(const std::string& url) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if(res != CURLE_OK) {
            return "";
        }
    }
    return readBuffer;
}

std::string extractTitle(const std::string& html) {
    std::string lowerHtml = html;
    std::transform(lowerHtml.begin(), lowerHtml.end(), lowerHtml.begin(), ::tolower);

    size_t start = lowerHtml.find("<title");
    if(start == std::string::npos) return "";

    start = lowerHtml.find(">", start);
    if(start == std::string::npos) return "";
    start++;

    size_t end = lowerHtml.find("</title>", start);
    if(end == std::string::npos) return "";

    std::string title = html.substr(start, end - start);

    size_t first = title.find_first_not_of(" \t\n\r");
    if(first == std::string::npos) return "";
    size_t last = title.find_last_not_of(" \t\n\r");
    return title.substr(first, last - first + 1);
}

int main(int argc, char* argv[]) {
    if(argc < 2) {
        return 1;
    }

    std::string url = argv[1];
    std::string html = fetchUrl(url);

    if(html.empty()) {
        return 1;
    }

    std::string title = extractTitle(html);
    std::cout << title << std::endl;

    return 0;
}