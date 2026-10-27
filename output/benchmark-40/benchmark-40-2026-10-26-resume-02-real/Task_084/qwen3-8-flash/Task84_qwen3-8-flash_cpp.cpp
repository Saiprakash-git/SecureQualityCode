#include <iostream>
#include <string>
#include <random>
#include <unordered_set>
#include <mutex>
#include <cstddef>
#include <cstdint>

std::string generateSessionId(std::mt19937_64& rng, std::unordered_set<std::string>& usedSessionIds, std::mutex& usedSessionIdsMutex, std::size_t length) {
    static const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::lock_guard<std::mutex> lock(usedSessionIdsMutex);
    std::uniform_int_distribution<std::size_t> distribution(0, sizeof(charset) - 2);

    while (true) {
        std::string sessionId;
        sessionId.reserve(length);

        for (std::size_t i = 0; i < length; ++i) {
            sessionId.push_back(charset[distribution(rng)]);
        }

        if (usedSessionIds.insert(sessionId).second) {
            return sessionId;
        }
    }
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    std::random_device randomDevice;
    std::uint64_t seed = static_cast<std::uint64_t>(randomDevice()) << 32;
    seed ^= static_cast<std::uint64_t>(randomDevice());

    std::mt19937_64 rng(seed);
    std::unordered_set<std::string> usedSessionIds;
    std::mutex usedSessionIdsMutex;

    std::string sessionId = generateSessionId(rng, usedSessionIds, usedSessionIdsMutex, 32);

    std::cout << sessionId << '\n';

    return 0;
}