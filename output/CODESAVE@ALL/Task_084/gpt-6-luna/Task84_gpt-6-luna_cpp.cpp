#include <array>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <random>
#include <sstream>
#include <string>
#include <unordered_set>

std::string generateSessionId() {
    static std::mutex mutex;
    static std::unordered_set<std::string> issuedIds;
    static std::mt19937_64 generator([] {
        std::random_device device;
        std::seed_seq seed{
            device(), device(), device(), device(),
            device(), device(), device(), device()
        };
        return std::mt19937_64(seed);
    }());

    std::lock_guard<std::mutex> lock(mutex);

    for (;;) {
        std::array<std::uint8_t, 16> bytes{};
        for (std::size_t i = 0; i < bytes.size(); i += 8) {
            const std::uint64_t value = generator();
            for (std::size_t j = 0; j < 8; ++j) {
                bytes[i + j] = static_cast<std::uint8_t>(value >> (j * 8));
            }
        }

        bytes[6] = static_cast<std::uint8_t>((bytes[6] & 0x0F) | 0x40);
        bytes[8] = static_cast<std::uint8_t>((bytes[8] & 0x3F) | 0x80);

        std::ostringstream id;
        id << std::hex << std::setfill('0');
        for (std::size_t i = 0; i < bytes.size(); ++i) {
            id << std::setw(2) << static_cast<unsigned int>(bytes[i]);
            if (i == 3 || i == 5 || i == 7 || i == 9) {
                id << '-';
            }
        }

        std::string result = id.str();
        if (issuedIds.insert(result).second) {
            return result;
        }
    }
}

int main() {
    std::cout << generateSessionId() << '\n';
    return 0;
}