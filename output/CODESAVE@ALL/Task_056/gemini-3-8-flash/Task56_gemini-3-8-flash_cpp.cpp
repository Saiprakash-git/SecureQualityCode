#include <iostream>
#include <string>
#include <vector>
#include <cstdint>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <random>

class SHA256 {
public:
    static std::vector<uint8_t> hash(const std::vector<uint8_t>& data) {
        uint32_t h[8] = {
            0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
            0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
        };

        static const uint32_t k[64] = {
            0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
            0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
            0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
            0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
            0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
            0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
            0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
            0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
        };

        std::vector<uint8_t> msg = data;
        uint64_t bit_len = static_cast<uint64_t>(msg.size()) * 8;

        msg.push_back(0x80);
        while ((msg.size() + 8) % 64 != 0) {
            msg.push_back(0x00);
        }

        for (int i = 7; i >= 0; --i) {
            msg.push_back(static_cast<uint8_t>((bit_len >> (i * 8)) & 0xff));
        }

        for (size_t chunk = 0; chunk < msg.size(); chunk += 64) {
            uint32_t w[64];
            for (size_t i = 0; i < 16; ++i) {
                w[i] = (static_cast<uint32_t>(msg[chunk + i * 4]) << 24) |
                       (static_cast<uint32_t>(msg[chunk + i * 4 + 1]) << 16) |
                       (static_cast<uint32_t>(msg[chunk + i * 4 + 2]) << 8) |
                       (static_cast<uint32_t>(msg[chunk + i * 4 + 3]));
            }

            for (size_t i = 16; i < 64; ++i) {
                uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
                uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
                w[i] = w[i - 16] + s0 + w[i - 7] + s1;
            }

            uint32_t a = h[0], b = h[1], c = h[2], d = h[3];
            uint32_t e = h[4], f = h[5], g = h[6], nh = h[7];

            for (size_t i = 0; i < 64; ++i) {
                uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
                uint32_t ch = (e & f) ^ ((~e) & g);
                uint32_t temp1 = nh + S1 + ch + k[i] + w[i];
                uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
                uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
                uint32_t temp2 = S0 + maj;

                nh = g;
                g = f;
                f = e;
                e = d + temp1;
                d = c;
                c = b;
                b = a;
                a = temp1 + temp2;
            }

            h[0] += a;
            h[1] += b;
            h[2] += c;
            h[3] += d;
            h[4] += e;
            h[5] += f;
            h[6] += g;
            h[7] += nh;
        }

        std::vector<uint8_t> digest(32);
        for (size_t i = 0; i < 8; ++i) {
            digest[i * 4] = static_cast<uint8_t>((h[i] >> 24) & 0xff);
            digest[i * 4 + 1] = static_cast<uint8_t>((h[i] >> 16) & 0xff);
            digest[i * 4 + 2] = static_cast<uint8_t>((h[i] >> 8) & 0xff);
            digest[i * 4 + 3] = static_cast<uint8_t>(h[i] & 0xff);
        }
        return digest;
    }

private:
    static inline uint32_t rotr(uint32_t x, uint32_t n) {
        return (x >> n) | (x << (32 - n));
    }
};

class HMAC_SHA256 {
public:
    static std::vector<uint8_t> compute(const std::vector<uint8_t>& key, const std::vector<uint8_t>& message) {
        std::vector<uint8_t> k = key;
        if (k.size() > 64) {
            k = SHA256::hash(k);
        }
        k.resize(64, 0x00);

        std::vector<uint8_t> i_key_pad(64);
        std::vector<uint8_t> o_key_pad(64);

        for (size_t i = 0; i < 64; ++i) {
            i_key_pad[i] = k[i] ^ 0x36;
            o_key_pad[i] = k[i] ^ 0x5c;
        }

        std::vector<uint8_t> inner_data = i_key_pad;
        inner_data.insert(inner_data.end(), message.begin(), message.end());
        std::vector<uint8_t> inner_hash = SHA256::hash(inner_data);

        std::vector<uint8_t> outer_data = o_key_pad;
        outer_data.insert(outer_data.end(), inner_hash.begin(), inner_hash.end());
        return SHA256::hash(outer_data);
    }
};

class Base64URL {
public:
    static std::string encode(const std::vector<uint8_t>& data) {
        static const char tbl[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
        std::string result;
        size_t len = data.size();
        for (size_t i = 0; i < len; i += 3) {
            uint32_t val = (static_cast<uint32_t>(data[i]) << 16);
            if (i + 1 < len) val |= (static_cast<uint32_t>(data[i + 1]) << 8);
            if (i + 2 < len) val |= static_cast<uint32_t>(data[i + 2]);

            result.push_back(tbl[(val >> 18) & 0x3f]);
            result.push_back(tbl[(val >> 12) & 0x3f]);
            if (i + 1 < len) result.push_back(tbl[(val >> 6) & 0x3f]);
            if (i + 2 < len) result.push_back(tbl[val & 0x3f]);
        }
        return result;
    }

    static std::vector<uint8_t> decode(const std::string& input) {
        std::vector<uint8_t> out;
        std::vector<int> T(256, -1);
        const char tbl[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
        for (int i = 0; i < 64; ++i) T[static_cast<uint8_t>(tbl[i])] = i;

        uint32_t val = 0;
        int valb = -8;
        for (char c : input) {
            if (T[static_cast<uint8_t>(c)] == -1) break;
            val = (val << 6) + T[static_cast<uint8_t>(c)];
            valb += 6;
            if (valb >= 0) {
                out.push_back(static_cast<uint8_t>((val >> valb) & 0xff));
                valb -= 8;
            }
        }
        return out;
    }
};

std::string generate_random_hex(size_t bytes) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dist(0, 255);
    std::stringstream ss;
    for (size_t i = 0; i < bytes; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << dist(gen);
    }
    return ss.str();
}

std::string generate_auth_token(const std::string& user_id, uint64_t ttl_seconds, const std::string& secret_key) {
    auto now = std::chrono::system_clock::now();
    uint64_t expiry = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count() + ttl_seconds;
    std::string nonce = generate_random_hex(8);

    std::ostringstream payload_stream;
    payload_stream << user_id << ":" << expiry << ":" << nonce;
    std