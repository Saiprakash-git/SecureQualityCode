#include <iostream>
#include <string>
#include <random>
#include <unordered_set>
#include <chrono>
#include <sstream>
#include <iomanip>

std::string generateSessionId(std::mt19937_64& rng) {
    static const char charset[] =
        "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const size_t charsetSize = sizeof(charset) - 1;
    std::uniform_int_distribution<size_t> dist(0, charsetSize - 1);

    std::string id;
    id.reserve(32);
    for (int i = 0; i < 32; ++i) {
        id.push_back(charset[dist(rng)]);
    }
    return id;
}

std::string generateUniqueSessionId(std::unordered_set<std::string>& existing, std::mt19937_64& rng) {
    std::string id;
    do {
        id = generateSessionId(rng);
    } while (existing.count(id) > 0);
    existing.insert(id);
    return id;
}

int main() {
    std::random_device rd;
    std::seed_seq seq{
        rd(), rd(), rd(), rd(),
        static_cast<unsigned int>(std::chrono::high_resolution_clock::now().time_since_epoch().count())
    };
    std::mt19937_64 rng(seq);

    std::unordered_set<std::string> issuedIds;

    std::string sessionId = generateUniqueSessionId(issuedIds, rng);
    std::cout << sessionId << std::endl;

    return 0;
}