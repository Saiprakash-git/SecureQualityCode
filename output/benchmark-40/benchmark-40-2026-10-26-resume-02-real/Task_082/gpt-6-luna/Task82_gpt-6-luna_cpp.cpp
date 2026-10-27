#include <array>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using Byte = std::uint8_t;
using Digest = std::array<Byte, 32>;

static std::uint32_t rotateRight(std::uint32_t value, unsigned int bits) {
    return (value >> bits) | (value << (32 - bits));
}

static Digest sha256(const std::vector<Byte>& input) {
    static constexpr std::array<std::uint32_t, 64> constants = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
        0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
        0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
        0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
        0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
        0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
        0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
        0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
        0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

    std::vector<Byte> message = input;
    const std::uint64_t bitLength = static_cast<std::uint64_t>(message.size()) * 8;
    message.push_back(0x80);
    while ((message.size() % 64) != 56) {
        message.push_back(0);
    }
    for (int i = 7; i >= 0; --i) {
        message.push_back(static_cast<Byte>(bitLength >> (i * 8)));
    }

    std::array<std::uint32_t, 8> state = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };

    for (std::size_t offset = 0; offset < message.size(); offset += 64) {
        std::array<std::uint32_t, 64> words{};
        for (int i = 0; i < 16; ++i) {
            const std::size_t j = offset + static_cast<std::size_t>(i) * 4;
            words[i] = (static_cast<std::uint32_t>(message[j]) << 24) |
                       (static_cast<std::uint32_t>(message[j + 1]) << 16) |
                       (static_cast<std::uint32_t>(message[j + 2]) << 8) |
                       static_cast<std::uint32_t>(message[j + 3]);
        }
        for (int i = 16; i < 64; ++i) {
            const std::uint32_t s0 = rotateRight(words[i - 15], 7) ^
                                     rotateRight(words[i - 15], 18) ^
                                     (words[i - 15] >> 3);
            const std::uint32_t s1 = rotateRight(words[i - 2], 17) ^
                                     rotateRight(words[i - 2], 19) ^
                                     (words[i - 2] >> 10);
            words[i] = words[i - 16] + s0 + words[i - 7] + s1;
        }

        std::uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
        std::uint32_t e = state[4], f = state[5], g = state[6], h = state[7];

        for (int i = 0; i < 64; ++i) {
            const std::uint32_t sum1 = rotateRight(e, 6) ^ rotateRight(e, 11) ^ rotateRight(e, 25);
            const std::uint32_t choice = (e & f) ^ (~e & g);
            const std::uint32_t temp1 = h + sum1 + choice + constants[i] + words[i];
            const std::uint32_t sum0 = rotateRight(a, 2) ^ rotateRight(a, 13) ^ rotateRight(a, 22);
            const std::uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
            const std::uint32_t temp2 = sum0 + majority;

            h = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }

        state[0] += a;
        state[1] += b;
        state[2] += c;
        state[3] += d;
        state[4] += e;
        state[5] += f;
        state[6] += g;
        state[7] += h;
    }

    Digest digest{};
    for (std::size_t i = 0; i < state.size(); ++i) {
        digest[i * 4] = static_cast<Byte>(state[i] >> 24);
        digest[i * 4 + 1] = static_cast<Byte>(state[i] >> 16);
        digest[i * 4 + 2] = static_cast<Byte>(state[i] >> 8);
        digest[i * 4 + 3] = static_cast<Byte>(state[i]);
    }
    return digest;
}

static Digest hmacSha256(const std::vector<Byte>& key, const std::vector<Byte>& message) {
    constexpr std::size_t blockSize = 64;
    std::vector<Byte> normalizedKey = key;

    if (normalizedKey.size() > blockSize) {
        const Digest hashedKey = sha256(normalizedKey);
        normalizedKey.assign(hashedKey.begin(), hashedKey.end());
    }
    normalizedKey.resize(blockSize, 0);

    std::vector<Byte> inner(blockSize);
    std::vector<Byte> outer(blockSize);
    for (std::size_t i = 0; i < blockSize; ++i) {
        inner[i] = normalizedKey[i] ^ 0x36;
        outer[i] = normalizedKey[i] ^ 0x5c;
    }
    inner.insert(inner.end(), message.begin(), message.end());
    const Digest innerDigest = sha256(inner);

    outer.insert(outer.end(), innerDigest.begin(), innerDigest.end());
    return sha256(outer);
}

static std::string hashPassword(const std::string& password, const std::string& salt) {
    constexpr std::uint32_t iterations = 100000;

    const std::vector<Byte> key(password.begin(), password.end());
    std::vector<Byte> saltBytes(salt.begin(), salt.end());
    saltBytes.insert(saltBytes.end(), {0, 0, 0, 1});

    Digest block = hmacSha256(key, saltBytes);
    Digest result = block;

    for (std::uint32_t i = 1; i < iterations; ++i) {
        const std::vector<Byte> previous(block.begin(), block.end());
        block = hmacSha256(key, previous);
        for (std::size_t j = 0; j < result.size(); ++j) {
            result[j] ^= block[j];
        }
    }

    std::ostringstream output;
    output << std::hex << std::setfill('0');
    for (Byte value : result) {
        output << std::setw(2) << static_cast<unsigned int>(value);
    }
    return output.str();
}

int main() {
    std::string password;
    std::string salt;

    if (!std::getline(std::cin, password) || !std::getline(std::cin, salt)) {
        return 1;
    }

    std::cout << hashPassword(password, salt) << '\n';
    return 0;
}