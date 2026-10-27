#include <iostream>
#include <random>
#include <string>
#include <iomanip>
#include <sstream>
#include <cstdint>

std::string generateSessionId(std::mt19937_64& rng) {
    std::uniform_int_distribution<uint64_t> dist;
    uint64_t d1 = dist(rng);
    uint64_t d2 = dist(rng);

    // RFC 4122 compliance for UUID version 4
    d1 = (d1 & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL; // Version 4
    d2 = (d2 & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL; // Variant 1

    std::ostringstream ss;
    ss << std::hex << std::setfill('0')
       << std::setw(8) << (d1 >> 32) << "-"
       << std::setw(4) << ((d1 >> 16) & 0xFFFF) << "-"
       << std::setw(4) << (d1 & 0xFFFF) << "-"
       << std::setw(4) << (d2 >> 48) << "-"
       << std::setw(12) << (d2 & 0xFFFFFFFFFFFFULL);

    return ss.str();
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    std::random_device rd;
    std::mt19937_64 rng(rd());

    std::string sessionId = generateSessionId(rng);
    std::cout << sessionId << std::endl;

    return 0;
}