#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <cstdint>

namespace sha256 {
    inline uint32_t rotr(uint32_t x, uint32_t n) {
        return (x >> n) | (x << (32 - n));
    }

    inline uint32_t choose(uint32_t e, uint32_t f, uint32_t g) {
        return (e & f) ^ (~e & g);
    }

    inline uint32_t majority(uint32_t a, uint32_t b, uint32_t c) {
        return (a & b) ^ (a & c) ^ (b & c);
    }

    inline uint32_t sig0(uint32_t x) {
        return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
    }

    inline uint32_t sig1(uint32_t x) {
        return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
    }

    inline uint32_t ep0(uint32_t x) {
        return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
    }

    inline uint32_t ep1(uint32_t x) {
        return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
    }

    static const uint32_t K[64] = {
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

    std::string hash(const std::string& input) {
        uint32_t h0 = 0x6a09e667;
        uint32_t h1 = 0xbb67ae85;
        uint32_t h2 = 0x3c6ef372;
        uint32_t h3 = 0xa54ff53a;
        uint32_t h4 = 0x510e527f;
        uint32_t h5 = 0x9b05688c;
        uint32_t h6 = 0x1f83d9ab;
        uint32_t h7 = 0x5be0cd19;

        std::vector<uint8_t> data(input.begin(), input.end());
        uint64_t bit_len = data.size() * 8;

        data.push_back(0x80);
        while ((data.size() % 64) != 56) {
            data.push_back(0x00);
        }

        for (int i = 7; i >= 0; --i) {
            data.push_back(static_cast<uint8_t>((bit_len >> (i * 8)) & 0xff));
        }

        for (size_t chunk = 0; chunk < data.size(); chunk += 64) {
            uint32_t w[64];
            for (int i = 0; i < 16; ++i) {
                w[i] = (static_cast<uint32_t>(data[chunk + i * 4]) << 24) |
                       (static_cast<uint32_t>(data[chunk + i * 4 + 1]) << 16) |
                       (static_cast<uint32_t>(data[chunk + i * 4 + 2]) << 8) |
                       (static_cast<uint32_t>(data[chunk + i * 4 + 3]));
            }
            for (int i = 16; i < 64; ++i) {
                w[i] = sig1(w[i - 2]) + w[i - 7] + sig0(w[i - 15]) + w[i - 16];
            }

            uint32_t a = h0;
            uint32_t b = h1;
            uint32_t c = h2;
            uint32_t d = h3;
            uint32_t e = h4;
            uint32_t f = h5;
            uint32_t g = h6;
            uint32_t h = h7;

            for (int i = 0; i < 64; ++i) {
                uint32_t temp1 = h + ep1(e) + choose(e, f, g) + K[i] + w[i];
                uint32_t temp2 = ep0(a) + majority(a, b, c);
                h = g;
                g = f;
                f = e;
                e = d + temp1;
                d = c;
                c = b;
                b = a;
                a = temp1 + temp2;
            }

            h0 += a;
            h1 += b;
            h2 += c;
            h3 += d;
            h4 += e;
            h5 += f;
            h6 += g;
            h7 += h;
        }

        std::stringstream ss;
        uint32_t hashes[8] = {h0, h1, h2, h3, h4, h5, h6, h7};
        for (int i = 0; i < 8; ++i) {
            ss << std::hex << std::setw(8) << std::setfill('0') << hashes[i];
        }
        return ss.str();
    }
}

std::string compute_salted_hash(const std::string& password, const std::string& salt) {
    return sha256::hash(salt + password);
}

int main() {
    std::string password = "SecretPassword123";
    std::string salt = "random_salt_value";

    std::string hashed = compute_salted_hash(password, salt);

    std::cout << "Password: " << password << std::endl;
    std::cout << "Salt:     " << salt << std::endl;
    std::cout << "Hash:     " << hashed << std::endl;

    return 0;
}