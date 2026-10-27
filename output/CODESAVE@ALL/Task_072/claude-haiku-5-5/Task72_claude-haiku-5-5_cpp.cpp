#include <cstdint>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace aes_cbc {

static const size_t BLOCK_SIZE = 16;

static uint8_t gmul(uint8_t a, uint8_t b) {
    uint8_t p = 0;
    while (b) {
        if (b & 1) p ^= a;
        bool hi = (a & 0x80) != 0;
        a <<= 1;
        if (hi) a ^= 0x1b;
        b >>= 1;
    }
    return p;
}

static uint8_t ginv(uint8_t a) {
    if (!a) return 0;
    uint8_t r = 1, base = a;
    int e = 254;
    while (e) {
        if (e & 1) r = gmul(r, base);
        base = gmul(base, base);
        e >>= 1;
    }
    return r;
}

static uint8_t rotl8(uint8_t x, int n) {
    return static_cast<uint8_t>((x << n) | (x >> (8 - n)));
}

struct SBox {
    uint8_t fwd[256];
    SBox() {
        for (int i = 0; i < 256; ++i) {
            uint8_t b = ginv(static_cast<uint8_t>(i));
            fwd[i] = b ^ rotl8(b, 1) ^ rotl8(b, 2) ^ rotl8(b, 3) ^ rotl8(b, 4) ^ 0x63;
        }
    }
};

static const SBox& sbox() {
    static const SBox instance;
    return instance;
}

static void keyExpansion(const std::vector<uint8_t>& key, std::vector<uint8_t>& rk, int& Nr) {
    int Nk = static_cast<int>(key.size() / 4);
    Nr = Nk + 6;
    int totalWords = 4 * (Nr + 1);
    rk.assign(static_cast<size_t>(totalWords) * 4, 0);

    for (int i = 0; i < 4 * Nk; ++i) rk[i] = key[i];

    uint8_t rcon = 1;
    for (int i = Nk; i < totalWords; ++i) {
        uint8_t t[4];
        for (int j = 0; j < 4; ++j) t[j] = rk[(i - 1) * 4 + j];

        if (i % Nk == 0) {
            uint8_t tmp = t[0];
            t[0] = sbox().fwd[t[1]] ^ rcon;
            t[1] = sbox().fwd[t[2]];
            t[2] = sbox().fwd[t[3]];
            t[3] = sbox().fwd[tmp];
            rcon = gmul(rcon, 2);
        } else if (Nk > 6 && i % Nk == 4) {
            for (int j = 0; j < 4; ++j) t[j] = sbox().fwd[t[j]];
        }

        for (int j = 0; j < 4; ++j)
            rk[i * 4 + j] = rk[(i - Nk) * 4 + j] ^ t[j];
    }
}

static void addRoundKey(uint8_t* s, const std::vector<uint8_t>& rk, int round) {
    for (int i = 0; i < 16; ++i) s[i] ^= rk[round * 16 + i];
}

static void subBytes(uint8_t* s) {
    for (int i = 0; i < 16; ++i) s[i] = sbox().fwd[s[i]];
}

static void shiftRows(uint8_t* s) {
    uint8_t t[16];
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            t[r + 4 * c] = s[r + 4 * ((c + r) % 4)];
    for (int i = 0; i < 16; ++i) s[i] = t[i];
}

static void mixColumns(uint8_t* s) {
    for (int c = 0; c < 4; ++c) {
        uint8_t a0 = s[4 * c], a1 = s[4 * c + 1], a2 = s[4 * c + 2], a3 = s[4 * c + 3];
        s[4 * c]     = gmul(a0, 2) ^ gmul(a1, 3) ^ a2 ^ a3;
        s[4 * c + 1] = a0 ^ gmul(a1, 2) ^ gmul(a2, 3) ^ a3;
        s[4 * c + 2] = a0 ^ a1 ^ gmul(a2, 2) ^ gmul(a3, 3);
        s[4 * c + 3] = gmul(a0, 3) ^ a1 ^ a2 ^ gmul(a3, 2);
    }
}

static void encryptBlock(uint8_t* block, const std::vector<uint8_t>& rk, int Nr) {
    addRoundKey(block, rk, 0);
    for (int round = 1; round < Nr; ++round) {
        subBytes(block);
        shiftRows(block);
        mixColumns(block);
        addRoundKey(block, rk, round);
    }
    subBytes(block);
    shiftRows(block);
    addRoundKey(block, rk, Nr);
}

static std::vector<uint8_t> pkcs7Pad(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> out = data;
    size_t padLen = BLOCK_SIZE - (data.size() % BLOCK_SIZE);
    out.insert(out.end(), padLen, static