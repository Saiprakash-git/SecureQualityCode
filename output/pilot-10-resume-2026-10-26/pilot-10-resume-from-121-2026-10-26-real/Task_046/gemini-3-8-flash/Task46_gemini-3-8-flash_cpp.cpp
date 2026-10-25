#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <regex>
#include <sstream>
#include <iomanip>
#include <random>
#include <cstdint>
#include <cstring>

namespace Crypto {
    inline uint32_t rightRotate(uint32_t value, unsigned int count) {
        return (value >> count) | (value << (32 - count));
    }

    std::string sha256(const std::string& input) {
        const uint32_t K[64] = {
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

        uint32_t H[8] = {
            0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
            0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
        };

        std::vector<uint8_t> data(input.begin(), input.end());
        uint64_t bitLength = data.size() * 8;

        data.push_back(0x80);
        while ((data.size() % 64) != 56) {
            data.push_back(0x00);
        }

        for (int i = 7; i >= 0; --i) {
            data.push_back(static_cast<uint8_t>((bitLength >> (i * 8)) & 0xFF));
        }

        for (size_t chunk = 0; chunk < data.size(); chunk += 64) {
            uint32_t W[64];
            for (int t = 0; t < 16; ++t) {
                W[t] = (static_cast<uint32_t>(data[chunk + t * 4]) << 24) |
                       (static_cast<uint32_t>(data[chunk + t * 4 + 1]) << 16) |
                       (static_cast<uint32_t>(data[chunk + t * 4 + 2]) << 8) |
                       (static_cast<uint32_t>(data[chunk + t * 4 + 3]));
            }

            for (int t = 16; t < 64; ++t) {
                uint32_t s0 = rightRotate(W[t - 15], 7) ^ rightRotate(W[t - 15], 18) ^ (W[t - 15] >> 3);
                uint32_t s1 = rightRotate(W[t - 2], 17) ^ rightRotate(W[t - 2], 19) ^ (W[t - 2] >> 10);
                W[t] = W[t - 16] + s0 + W[t - 7] + s1;
            }

            uint32_t a = H[0], b = H[1], c = H[2], d = H[3];
            uint32_t e = H[4], f = H[5], g = H[6], h = H[7];

            for (int t = 0; t < 64; ++t) {
                uint32_t S1 = rightRotate(e, 6) ^ rightRotate(e, 11) ^ rightRotate(e, 25);
                uint32_t ch = (e & f) ^ ((~e) & g);
                uint32_t temp1 = h + S1 + ch + K[t] + W[t];
                uint32_t S0 = rightRotate(a, 2) ^ rightRotate(a, 13) ^ rightRotate(a, 22);
                uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
                uint32_t temp2 = S0 + maj;

                h = g;
                g = f;
                f = e;
                e = d + temp1;
                d = c;
                c = b;
                b = a;
                a = temp1 + temp2;
            }

            H[0] += a;
            H[1] += b;
            H[2] += c;
            H[3] += d;
            H[4] += e;
            H[5] += f;
            H[6] += g;
            H[7] += h;
        }

        std::ostringstream result;
        for (int i = 0; i < 8; ++i) {
            result << std::hex << std::setfill('0') << std::setw(8) << H[i];
        }
        return result.str();
    }

    std::string generateSalt(size_t length = 16) {
        const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        std::random_device rd;
        std::mt19937 generator(rd());
        std::uniform_int_distribution<size_t> dist(0, sizeof(charset) - 2);

        std::string salt;
        salt.reserve(length);
        for (size_t i = 0; i < length; ++i) {
            salt += charset[dist(generator)];
        }
        return salt;
    }
}

struct UserRecord {
    uint64_t id;
    std::string name;
    std::string email;
    std::string passwordHash;
    std::string salt;
};

class Database {
private:
    uint64_t nextId;
    std::unordered_map<uint64_t, UserRecord> recordsById;
    std::unordered_map<std::string, uint64_t> emailIndex;

public:
    Database() : nextId(1) {}

    bool emailExists(const std::string& email) const {
        return emailIndex.find(email) != emailIndex.end();
    }

    bool insertUser(const std::string& name, const std::string& email, const std::string& passwordHash, const std::string& salt, uint64_t& outId) {
        if (emailExists(email)) {
            return false;
        }

        uint64_t id = nextId++;
        UserRecord record{id, name, email, passwordHash, salt};
        recordsById[id] = record;
        emailIndex[email] = id;
        outId = id;
        return true;
    }

    bool getUserById(uint64_t id, UserRecord& outRecord) const {
        auto it = recordsById.find(id);
        if (it != recordsById.end()) {
            outRecord = it->second;
            return true;
        }
        return false;
    }

    size_t count() const {
        return recordsById.size();
    }
};

class RegistrationService {
public:
    enum class Status {
        Success,
        InvalidName,
        InvalidEmail,
        WeakPassword,
        EmailAlreadyExists,
        DatabaseError
    };

    static bool isValidName(const std::string& name) {
        return name.length() >= 2 && name.length() <= 100;
    }

    static bool isValidEmail(const std::string& email) {
        const std::regex emailRegex(R"(^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$)");
        return std::regex_match(email, emailRegex);
    }

    static bool isValidPassword(const std::string& password) {
        if (password.length() < 8) return false;

        bool hasUpper = false;
        bool hasLower = false;
        bool hasDigit = false;
        bool hasSpecial = false;

        for (char ch : password) {
            if (std::isupper(static_cast<unsigned char>(ch))) hasUpper = true;
            else if (std::islower(static_cast<unsigned char>(ch))) hasLower = true;
            else if (std::isdigit(static_cast<unsigned char>(ch))) hasDigit = true;
            else hasSpecial = true;
        }

        return hasUpper && hasLower && hasDigit && hasSpecial;
    }

    static Status registerUser(Database& db, const std::string& name, const std::string& email, const std::string& password, uint64_t& createdUserId) {
        if (!isValidName(name)) {
            return Status::InvalidName;
        }

        if (!isValidEmail(email)) {
            return Status::InvalidEmail;
        }

        if (!isValidPassword(password)) {
            return Status::WeakPassword;
        }

        if (db.emailExists(email)) {
            return Status::EmailAlreadyExists;
        }

        std::string salt = Crypto::generateSalt(16);
        std::string hashedPassword = Crypto::sha256(password + salt);

        bool inserted = db.insertUser(name, email, hashedPassword, salt, createdUserId);
        if (!inserted) {
            return Status::DatabaseError;
        }

        return Status::Success;
    }

    static std::string statusToString(Status status) {
        switch (status) {
            case Status::Success: return "Registration successful.";
            case Status::InvalidName: return "Error: Name must be between 2 and 100 characters.";
            case Status::InvalidEmail: return "Error: Invalid email address format.";
            case Status::WeakPassword: return "Error: Password must be at least 8 characters and contain uppercase