#include <array>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace {

constexpr std::uint32_t kPbkdf2Iterations = 120000;

class Sha256 {
public:
    void update(const std::uint8_t* data, std::size_t length) {
        totalBytes_ += length;
        while (length > 0) {
            const std::size_t copied = std::min(length, block_.size() - blockSize_);
            for (std::size_t i = 0; i < copied; ++i) {
                block_[blockSize_ + i] = data[i];
            }
            blockSize_ += copied;
            data += copied;
            length -= copied;

            if (blockSize_ == block_.size()) {
                transform(block_.data());
                blockSize_ = 0;
            }
        }
    }

    std::array<std::uint8_t, 32> finish() {
        const std::uint64_t bitLength = static_cast<std::uint64_t>(totalBytes_) * 8;
        block_[blockSize_++] = 0x80;

        if (blockSize_ > 56) {
            while (blockSize_ < 64) {
                block_[blockSize_++] = 0;
            }
            transform(block_.data());
            blockSize_ = 0;
        }

        while (blockSize_ < 56) {
            block_[blockSize_++] = 0;
        }

        for (int i = 7; i >= 0; --i) {
            block_[blockSize_++] =
                static_cast<std::uint8_t>((bitLength >> (i * 8)) & 0xff);
        }
        transform(block_.data());

        std::array<std::uint8_t, 32> digest{};
        for (std::size_t i = 0; i < state_.size(); ++i) {
            digest[i * 4] = static_cast<std::uint8_t>(state_[i] >> 24);
            digest[i * 4 + 1] = static_cast<std::uint8_t>(state_[i] >> 16);
            digest[i * 4 + 2] = static_cast<std::uint8_t>(state_[i] >> 8);
            digest[i * 4 + 3] = static_cast<std::uint8_t>(state_[i]);
        }
        return digest;
    }

private:
    static std::uint32_t rotateRight(std::uint32_t value, unsigned int amount) {
        return (value >> amount) | (value << (32 - amount));
    }

    void transform(const std::uint8_t* block) {
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

        std::array<std::uint32_t, 64> words{};
        for (std::size_t i = 0; i < 16; ++i) {
            const std::size_t offset = i * 4;
            words[i] = (static_cast<std::uint32_t>(block[offset]) << 24) |
                       (static_cast<std::uint32_t>(block[offset + 1]) << 16) |
                       (static_cast<std::uint32_t>(block[offset + 2]) << 8) |
                       static_cast<std::uint32_t>(block[offset + 3]);
        }

        for (std::size_t i = 16; i < 64; ++i) {
            const std::uint32_t s0 =
                rotateRight(words[i - 15], 7) ^
                rotateRight(words[i - 15], 18) ^
                (words[i - 15] >> 3);
            const std::uint32_t s1 =
                rotateRight(words[i - 2], 17) ^
                rotateRight(words[i - 2], 19) ^
                (words[i - 2] >> 10);
            words[i] = words[i - 16] + s0 + words[i - 7] + s1;
        }

        std::uint32_t a = state_[0];
        std::uint32_t b = state_[1];
        std::uint32_t c = state_[2];
        std::uint32_t d = state_[3];
        std::uint32_t e = state_[4];
        std::uint32_t f = state_[5];
        std::uint32_t g = state_[6];
        std::uint32_t h = state_[7];

        for (std::size_t i = 0; i < 64; ++i) {
            const std::uint32_t sum1 =
                rotateRight(e, 6) ^ rotateRight(e, 11) ^ rotateRight(e, 25);
            const std::uint32_t choice = (e & f) ^ (~e & g);
            const std::uint32_t temp1 = h + sum1 + choice + constants[i] + words[i];
            const std::uint32_t sum0 =
                rotateRight(a, 2) ^ rotateRight(a, 13) ^ rotateRight(a, 22);
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

        state_[0] += a;
        state_[1] += b;
        state_[2] += c;
        state_[3] += d;
        state_[4] += e;
        state_[5] += f;
        state_[6] += g;
        state_[7] += h;
    }

    std::array<std::uint32_t, 8> state_ = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };
    std::array<std::uint8_t, 64> block_{};
    std::size_t blockSize_ = 0;
    std::size_t totalBytes_ = 0;
};

std::array<std::uint8_t, 32> sha256(const std::vector<std::uint8_t>& data) {
    Sha256 hash;
    hash.update(data.data(), data.size());
    return hash.finish();
}

std::array<std::uint8_t, 32> hmacSha256(
    const std::string& password,
    const std::vector<std::uint8_t>& message) {
    std::array<std::uint8_t, 64> key{};
    if (password.size() > key.size()) {
        std::vector<std::uint8_t> passwordBytes(password.begin(), password.end());
        const auto digest = sha256(passwordBytes);
        std::copy(digest.begin(), digest.end(), key.begin());
    } else {
        std::copy(password.begin(), password.end(), key.begin());
    }

    std::vector<std::uint8_t> inner(64);
    std::vector<std::uint8_t> outer(64);
    for (std::size_t i = 0; i < key.size(); ++i) {
        inner[i] = key[i] ^ 0x36;
        outer[i] = key[i] ^ 0x5c;
    }
    inner.insert(inner.end(), message.begin(), message.end());
    const auto innerDigest = sha256(inner);
    outer.insert(outer.end(), innerDigest.begin(), innerDigest.end());
    return sha256(outer);
}

std::array<std::uint8_t, 32> derivePasswordHash(
    const std::string& password,
    const std::array<std::uint8_t, 16>& salt) {
    std::vector<std::uint8_t> message(salt.begin(), salt.end());
    message.insert(message.end(), {0, 0, 0, 1});

    auto u = hmacSha256(password, message);
    auto result = u;
    for (std::uint32_t i = 1; i < kPbkdf2Iterations; ++i) {
        std::vector<std::uint8_t> previous(u.begin(), u.end());
        u = hmacSha256(password, previous);
        for (std::size_t j = 0; j < result.size(); ++j) {
            result[j] ^= u[j];
        }
    }
    return result;
}

std::string toHex(const std::uint8_t* data, std::size_t length) {
    std::ostringstream result;
    result << std::hex << std::setfill('0');
    for (std::size_t i = 0; i < length; ++i) {
        result << std::setw(2) << static_cast<unsigned int>(data[i]);
    }
    return result.str();
}

std::string encodeText(const std::string& text) {
    return toHex(reinterpret_cast<const std::uint8_t*>(text.data()), text.size());
}

std::string lowerCase(std::string text) {
    for (char& c : text) {
        if (c >= 'A' && c <= 'Z') {
            c = static_cast<char>(c - 'A' + 'a');
        }
    }
    return text;
}

bool hasOnlyPrintableCharacters(const std::string& text) {
    for (unsigned char c : text) {
        if (c < 32 || c == 127) {
            return false;
        }
    }
    return true;
}

class UserDatabase {
public:
    explicit UserDatabase(std::string filePath) : filePath_(std::move(filePath)) {}

    bool registerUser(
        const std::string& name,
        const std::string& email,
        const std::string& password,
        std::string& error) const {
        if (name.empty() || email.empty() || password.empty()) {
            error = "All fields are required.";
            return false;
        }
        if (!hasOnlyPrintableCharacters(name) ||
            !hasOnlyPrintableCharacters(email) ||
            !hasOnlyPrintableCharacters(password)) {
            error = "Fields must not contain control characters.";
            return false;
        }
        if (email.find('@') == std::string::npos ||
            email.find('.') == std::string::npos) {
            error = "Please enter a valid email address.";
            return false;
        }
        if (password.size() < 8) {
            error = "Password must be at least 8 characters long.";
            return false;
        }
        if (emailExists(email, error)) {
            if (error.empty()) {
                error = "An account with this email already exists.";
            }
            return false;
        }
        if (!error.empty()) {
            return false;
        }

        std::array<std::uint8_t, 16> salt{};
        try {
            std::random_device random;
            for (std::uint8_t& byte : salt) {
                byte = static_cast<std::uint8_t>(random());
            }
        } catch (...) {
            error = "Unable to generate secure random salt.";
            return false;
        }

        const auto passwordHash = derivePasswordHash(password, salt);
        std::ofstream database(filePath_, std::ios::app);
        if (!database) {
            error = "Unable to open the user database.";
            return false;
        }

        database << encodeText(name) << '\t'
                 << encodeText(email) << '\t'
                 << kPbkdf2Iterations << '\t'
                 << toHex(salt.data(), salt.size()) << '\t'
                 << toHex(passwordHash.data(), passwordHash.size()) << '\n';
        if (!database) {
            error = "Unable to save the registration.";
            return false;
        }

        return true;
    }

private:
    bool emailExists(const std::string& email, std::string& error) const {
        std::ifstream database(filePath_);
        if (!database) {
            if (std::ifstream(filePath_).good()) {
                error = "Unable to read the user database.";
            }
            return false;
        }

        const std::string target = encodeText(lowerCase(email));
        std::string line;
        while (std::getline(database, line)) {
            const std::size_t firstTab = line.find('\t');
            const std::size_t secondTab =
                firstTab == std::string::npos ? firstTab : line.find('\t', firstTab + 1);
            if (firstTab == std::string::npos || secondTab == std::string::npos) {
                continue;
            }

            const std