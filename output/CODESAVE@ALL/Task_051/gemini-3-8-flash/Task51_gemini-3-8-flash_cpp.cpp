#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <array>

static inline uint32_t rotl32(uint32_t x, int n) {
    return (x << n) | (x >> (32 - n));
}

static inline void quarter_round(uint32_t& a, uint32_t& b, uint32_t& c, uint32_t& d) {
    a += b; d ^= a; d = rotl32(d, 16);
    c += d; b ^= c; b = rotl32(b, 12);
    a += b; d ^= a; d = rotl32(d, 8);
    c += d; b ^= c; b = rotl32(b, 7);
}

static inline uint32_t load32_le(const uint8_t* p) {
    return static_cast<uint32_t>(p[0]) |
          (static_cast<uint32_t>(p[1]) << 8) |
          (static_cast<uint32_t>(p[2]) << 16) |
          (static_cast<uint32_t>(p[3]) << 24);
}

static inline void store32_le(uint8_t* p, uint32_t val) {
    p[0] = static_cast<uint8_t>(val);
    p[1] = static_cast<uint8_t>(val >> 8);
    p[2] = static_cast<uint8_t>(val >> 16);
    p[3] = static_cast<uint8_t>(val >> 24);
}

std::array<uint8_t, 64> chacha20_block(const std::array<uint8_t, 32>& key,
                                       uint32_t counter,
                                       const std::array<uint8_t, 12>& nonce) {
    std::array<uint32_t, 16> state = {
        0x61707865, 0x3320646e, 0x79622d32, 0x6b206574,
        load32_le(key.data() + 0),
        load32_le(key.data() + 4),
        load32_le(key.data() + 8),
        load32_le(key.data() + 12),
        load32_le(key.data() + 16),
        load32_le(key.data() + 20),
        load32_le(key.data() + 24),
        load32_le(key.data() + 28),
        counter,
        load32_le(nonce.data() + 0),
        load32_le(nonce.data() + 4),
        load32_le(nonce.data() + 8)
    };

    std::array<uint32_t, 16> working_state = state;

    for (int i = 0; i < 10; ++i) {
        quarter_round(working_state[0], working_state[4],  working_state[8],  working_state[12]);
        quarter_round(working_state[1], working_state[5],  working_state[9],  working_state[13]);
        quarter_round(working_state[2], working_state[6],  working_state[10], working_state[14]);
        quarter_round(working_state[3], working_state[7],  working_state[11], working_state[15]);

        quarter_round(working_state[0], working_state[5],  working_state[10], working_state[15]);
        quarter_round(working_state[1], working_state[6],  working_state[11], working_state[12]);
        quarter_round(working_state[2], working_state[7],  working_state[8],  working_state[13]);
        quarter_round(working_state[3], working_state[4],  working_state[9],  working_state[14]);
    }

    std::array<uint8_t, 64> output{};
    for (int i = 0; i < 16; ++i) {
        store32_le(output.data() + (i * 4), working_state[i] + state[i]);
    }

    return output;
}

std::vector<uint8_t> chacha20_process(const std::vector<uint8_t>& input,
                                      const std::array<uint8_t, 32>& key,
                                      const std::array<uint8_t, 12>& nonce,
                                      uint32_t initial_counter = 1) {
    std::vector<uint8_t> output(input.size());
    size_t full_blocks = input.size() / 64;
    size_t remainder = input.size() % 64;
    uint32_t counter = initial_counter;

    for (size_t b = 0; b < full_blocks; ++b) {
        std::array<uint8_t, 64> keystream = chacha20_block(key, counter++, nonce);
        for (size_t i = 0; i < 64; ++i) {
            output[b * 64 + i] = input[b * 64 + i] ^ keystream[i];
        }
    }

    if (remainder > 0) {
        std::array<uint8_t, 64> keystream = chacha20_block(key, counter, nonce);
        for (size_t i = 0; i < remainder; ++i) {
            output[full_blocks * 64 + i] = input[full_blocks * 64 + i] ^ keystream[i];
        }
    }

    return output;
}

std::string bytes_to_hex(const std::vector<uint8_t>& data) {
    std::ostringstream oss;
    for (uint8_t byte : data) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }
    return oss.str();
}

std::string encrypt_string(const std::string& plaintext,
                           const std::array<uint8_t, 32>& key,
                           const std::array<uint8_t, 12>& nonce) {
    std::vector<uint8_t> input_bytes(plaintext.begin(), plaintext.end());
    std::vector<uint8_t> encrypted_bytes = chacha20_process(input_bytes, key, nonce, 1);
    return bytes_to_hex(encrypted_bytes);
}

std::string decrypt_string(const std::vector<uint8_t>& ciphertext,
                           const std::array<uint8_t, 32>& key,
                           const std::array<uint8_t, 12>& nonce) {
    std::vector<uint8_t> decrypted_bytes = chacha20_process(ciphertext, key, nonce, 1);
    return std::string(decrypted_bytes.begin(), decrypted_bytes.end());
}

int main() {
    std::string plaintext = "Hello, World! This is a confidential message encrypted via ChaCha20.";

    std::array<uint8_t, 32> key = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    };

    std::array<uint8_t, 12> nonce = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4a, 0x00, 0x00, 0x00, 0x00
    };

    std::string hex_cipher = encrypt_string(plaintext, key, nonce);

    std::cout << "Original Plaintext : " << plaintext << "\n";
    std::cout << "Ciphertext (Hex)   : " << hex_cipher << "\n";

    std::vector<uint8_t> raw_ciphertext;
    for (size_t i = 0; i < hex_cipher.length(); i += 2) {
        std::string byteString = hex_cipher.substr(i, 2);
        raw_ciphertext.push_back(static_cast<uint8_t>(std::stoi(byteString, nullptr, 16)));
    }

    std::string recovered = decrypt_string(raw_ciphertext, key, nonce);
    std::cout << "Decrypted Plaintext: " << recovered << "\n";

    return 0;
}