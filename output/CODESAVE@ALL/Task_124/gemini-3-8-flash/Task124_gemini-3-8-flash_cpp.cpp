#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdint>
#include <random>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <optional>

namespace SecureStorage {

    void secure_zero_memory(void* ptr, size_t size) {
        if (!ptr) return;
        volatile uint8_t* p = static_cast<volatile uint8_t*>(ptr);
        while (size--) {
            *p++ = 0;
        }
    }

    template <typename T>
    struct SecureAllocator {
        using value_type = T;

        SecureAllocator() noexcept = default;
        template <typename U>
        SecureAllocator(const SecureAllocator<U>&) noexcept {}

        T* allocate(size_t n) {
            return std::allocator<T>{}.allocate(n);
        }

        void deallocate(T* p, size_t n) noexcept {
            secure_zero_memory(p, n * sizeof(T));
            std::allocator<T>{}.deallocate(p, n);
        }
    };

    using SecureString = std::basic_string<char, std::char_traits<char>, SecureAllocator<char>>;
    using SecureByteVector = std::vector<uint8_t, SecureAllocator<uint8_t>>;

    inline uint32_t rotl32(uint32_t x, int n) {
        return (x << n) | (x >> (32 - n));
    }

    void chacha20_quarter_round(uint32_t& a, uint32_t& b, uint32_t& c, uint32_t& d) {
        a += b; d ^= a; d = rotl32(d, 16);
        c += d; b ^= c; b = rotl32(b, 12);
        a += b; d ^= a; d = rotl32(d, 8);
        c += d; b ^= c; b = rotl32(b, 7);
    }

    void chacha20_block(const uint32_t key[8], const uint32_t nonce[3], uint32_t counter, uint8_t out[64]) {
        uint32_t state[16] = {
            0x61707865, 0x3320646e, 0x79622d32, 0x6b206574,
            key[0], key[1], key[2], key[3],
            key[4], key[5], key[6], key[7],
            counter, nonce[0], nonce[1], nonce[2]
        };

        uint32_t working_state[16];
        std::memcpy(working_state, state, sizeof(state));

        for (int i = 0; i < 10; ++i) {
            chacha20_quarter_round(working_state[0], working_state[4], working_state[8],  working_state[12]);
            chacha20_quarter_round(working_state[1], working_state[5], working_state[9],  working_state[13]);
            chacha20_quarter_round(working_state[2], working_state[6], working_state[10], working_state[14]);
            chacha20_quarter_round(working_state[3], working_state[7], working_state[11], working_state[15]);
            chacha20_quarter_round(working_state[0], working_state[5], working_state[10], working_state[15]);
            chacha20_quarter_round(working_state[1], working_state[6], working_state[11], working_state[12]);
            chacha20_quarter_round(working_state[2], working_state[7], working_state[8],  working_state[13]);
            chacha20_quarter_round(working_state[3], working_state[4], working_state[9],  working_state[14]);
        }

        for (int i = 0; i < 16; ++i) {
            working_state[i] += state[i];
            out[i * 4 + 0] = static_cast<uint8_t>(working_state[i] & 0xFF);
            out[i * 4 + 1] = static_cast<uint8_t>((working_state[i] >> 8) & 0xFF);
            out[i * 4 + 2] = static_cast<uint8_t>((working_state[i] >> 16) & 0xFF);
            out[i * 4 + 3] = static_cast<uint8_t>((working_state[i] >> 24) & 0xFF);
        }

        secure_zero_memory(working_state, sizeof(working_state));
    }

    SecureByteVector chacha20_crypt(const SecureByteVector& key, const std::vector<uint8_t>& nonce, const SecureByteVector& input) {
        if (key.size() != 32 || nonce.size() != 12) {
            return {};
        }

        uint32_t k[8];
        for (int i = 0; i < 8; ++i) {
            k[i] = static_cast<uint32_t>(key[i * 4 + 0]) |
                   (static_cast<uint32_t>(key[i * 4 + 1]) << 8) |
                   (static_cast<uint32_t>(key[i * 4 + 2]) << 16) |
                   (static_cast<uint32_t>(key[i * 4 + 3]) << 24);
        }

        uint32_t n[3];
        for (int i = 0; i < 3; ++i) {
            n[i] = static_cast<uint32_t>(nonce[i * 4 + 0]) |
                   (static_cast<uint32_t>(nonce[i * 4 + 1]) << 8) |
                   (static_cast<uint32_t>(nonce[i * 4 + 2]) << 16) |
                   (static_cast<uint32_t>(nonce[i * 4 + 3]) << 24);
        }

        SecureByteVector output(input.size());
        uint8_t block[64];
        uint32_t counter = 1;

        for (size_t offset = 0; offset < input.size(); offset += 64) {
            chacha20_block(k, n, counter++, block);
            size_t bytes_to_xor = std::min(static_cast<size_t>(64), input.size() - offset);
            for (size_t j = 0; j < bytes_to_xor; ++j) {
                output[offset + j] = input[offset + j] ^ block[j];
            }
        }

        secure_zero_memory(k, sizeof(k));
        secure_zero_memory(n, sizeof(n));
        secure_zero_memory(block, sizeof(block));
        return output;
    }

    std::vector<uint8_t> generate_random_bytes(size_t length) {
        std::vector<uint8_t> bytes(length);
        std::random_device rd;
        for (size_t i = 0; i < length; ++i) {
            bytes[i] = static_cast<uint8_t>(rd() & 0xFF);
        }
        return bytes;
    }

    std::string mask_card_number(const std::string& card_num) {
        std::string digits;
        for (char c : card_num) {
            if (std::isdigit(static_cast<unsigned char>(c))) {
                digits += c;
            }
        }
        if (digits.length() < 4) {
            return std::string(card_num.length(), '*');
        }
        std::string masked;
        size_t visible_from = digits.length() - 4;
        size_t digit_idx = 0;
        for (char c : card_num) {
            if (std::isdigit(static_cast<unsigned char>(c))) {
                if (digit_idx >= visible_from) {
                    masked += c;
                } else {
                    masked += '*';
                }
                digit_idx++;
            } else {
                masked += c;
            }
        }
        return masked;
    }

    struct VaultRecord {
        std::string record_id;
        std::string masked_identifier;
        std::vector<uint8_t> nonce;
        std::vector<uint8_t> encrypted_payload;
    };

    class SensitiveDataVault {
    private:
        std::vector<VaultRecord> storage;

    public:
        bool store_record(const std::string& record_id,
                          const SecureString& sensitive_data,
                          const std::string& display_identifier,
                          const SecureByteVector& key) {
            if (key.size() != 32) {
                return false;
            }

            for (const auto& record : storage) {
                if (record.record_id == record_id) {
                    return false;
                }
            }

            std::vector<uint8_t> nonce = generate_random_bytes(12);
            SecureByteVector plaintext(sensitive_data.begin(), sensitive_data.end());
            SecureByteVector ciphertext = chacha20_crypt(key, nonce, plaintext);

            VaultRecord record;
            record.record_id = record_id;
            record.masked_identifier = mask_card_number(display_identifier);
            record.nonce = nonce;
            record.encrypted_payload = std::vector<uint8_t>(ciphertext.begin(), ciphertext.end());

            storage.push_back(std::move(record));
            return true;
        }

        std::optional<SecureString> retrieve_record(const std::string& record_id,
                                                   const SecureByteVector& key) const {
            if (key.size() != 32) {
                return std::nullopt;
            }

            for (const auto& record : storage) {
                if (record.record_id == record_id) {
                    SecureByteVector ciphertext(record.encrypted_payload.begin(), record.encrypted_payload.end());
                    SecureByteVector plaintext = chacha20_crypt(key, record.nonce, ciphertext);
                    SecureString result(plaintext.begin(), plaintext.end());
                    return result;
                }
            }
            return std::nullopt;
        }

        std::string get_masked_identifier(const std::string& record_id) const {
            for (const auto& record : storage) {
                if (record.record_id == record_id) {
                    return record.masked_identifier;
                }
            }
            return "";
        }

        bool delete_record(const std::string& record_id) {
            for (auto it = storage.begin(); it != storage.end(); ++it) {
                if (it->record_id == record_id) {
                    secure_zero_memory(it->encrypted_payload.data(), it->encrypted_payload.size());
                    secure_zero_memory(it->nonce.data(), it->nonce.size());
                    storage.erase(it);
                    return true;
                }
            }
            return false;
        }

        size_t count() const {
            return storage.size();
        }
    };
}

int main() {
    using namespace SecureStorage;

    std::vector<uint8_t> raw_key = generate_random_bytes(32);
    SecureByteVector master_key(raw_key.begin(), raw_key.end());
    secure_zero_memory(raw_key.data(), raw_key.size());

    SensitiveDataVault vault;

    std::string card_holder = "John Doe";
    std::string card_number = "4111-