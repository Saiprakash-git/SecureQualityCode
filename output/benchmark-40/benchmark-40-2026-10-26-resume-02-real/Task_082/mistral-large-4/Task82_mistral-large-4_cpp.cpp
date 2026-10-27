#include <string>
#include <vector>
#include <cstdint>
#include <sstream>
#include <iomanip>
#include <iostream>

class SHA256 {
private:
    static const uint32_t K[64];
    uint32_t H[8];
    std::vector<uint8_t> buffer;
    uint64_t totalLen;

    static uint32_t rotr(uint32_t x, uint32_t n) {
        return (x >> n) | (x << (32 - n));
    }

    void processBlock(const uint8_t* block) {
        uint32_t w[64];
        for (int i = 0; i < 16; i++) {
            w[i] = (block[i*4] << 24) | (block[i*4+1] << 16) | (block[i*4+2] << 8) | block[i*4+3];
        }
        for (int i = 16; i < 64; i++) {
            uint32_t s0 = rotr(w[i-15], 7) ^ rotr(w[i-15], 18) ^ (w[i-15] >> 3);
            uint32_t s1 = rotr(w[i-2], 17) ^ rotr(w[i-2], 19) ^ (w[i-2] >>