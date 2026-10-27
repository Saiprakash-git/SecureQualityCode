#include <array>
#include <climits>
#include <cstddef>
#include <map>
#include <mutex>
#include <string>
#include <string_view>

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

class UserAuthentication {
public:
    enum class RegistrationResult {
        success,
        invalid_username,
        weak_password,
        username_taken,
        cryptographic_error
    };

    UserAuthentication() {
        if (RAND_bytes(dummy_salt_.data(), static_cast<int>(dummy_salt_.size())) != 1 ||
            RAND_bytes(dummy_hash_.data(), static_cast<int>(dummy_hash_.size())) != 1) {
            throw std::runtime_error("Secure random number generation failed");
        }
    }

    UserAuthentication(const UserAuthentication&) = delete;
    UserAuthentication& operator=(const UserAuthentication&) = delete;

    ~UserAuthentication() {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& entry : users_) {
            OPENSSL_cleanse(entry.second.hash.data(), entry.second.hash.size());
            OPENSSL_cleanse(entry.second.salt.data(), entry.second.salt.size());
        }
        OPENSSL_cleanse(dummy_hash_.data(), dummy_hash_.size());
        OPENSSL_cleanse(dummy_salt_.data(), dummy_salt_.size());
    }

    RegistrationResult register_user(std::string_view username,
                                     std::string_view password) {
        if (!valid_username(username)) {
            return RegistrationResult::invalid_username;
        }
        if (!valid_password(password)) {
            return RegistrationResult::weak_password;
        }

        Record record;
        if (RAND_bytes(record.salt.data(),
                       static_cast<int>(record.salt.size())) != 1 ||
            !derive(password, record.salt, record.hash)) {
            OPENSSL_cleanse(record.salt.data(), record.salt.size());
            OPENSSL_cleanse(record.hash.data(), record.hash.size());
            return RegistrationResult::cryptographic_error;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        const auto result = users_.emplace(std::string(username), record);
        OPENSSL_cleanse(record.salt.data(), record.salt.size());
        OPENSSL_cleanse(record.hash.data(), record.hash.size());

        return result.second ? RegistrationResult::success
                             : RegistrationResult::username_taken;
    }

    bool authenticate(std::string_view username,
                      std::string_view password) const {
        if (password.size() > max_password_length ||
            password.size() > static_cast<std::size_t>(INT_MAX)) {
            return false;
        }

        Record record;
        bool found = false;
        if (valid_username(username)) {
            std::lock_guard<std::mutex> lock(mutex_);
            const auto it = users_.find(std::string(username));
            if (it != users_.end()) {
                record = it->second;
                found = true;
            }
        }

        if (!found) {
            record.salt = dummy_salt_;
        }

        std::array<unsigned char, hash_size> candidate{};
        const bool derived = derive(password, record.salt, candidate);
        const bool matches =
            derived &&
            CRYPTO_memcmp(candidate.data(),
                          found ? record.hash.data() : dummy_hash_.data(),
                          candidate.size()) == 0;

        OPENSSL_cleanse(candidate.data(), candidate.size());
        OPENSSL_cleanse(record.salt.data(), record.salt.size());
        OPENSSL_cleanse(record.hash.data(), record.hash.size());
        return found && matches;
    }

private:
    static constexpr int iterations = 310000;
    static constexpr std::size_t salt_size = 16;
    static constexpr std::size_t hash_size = 32;
    static constexpr std::size_t min_password_length = 12;
    static constexpr std::size_t max_password_length = 1024;

    struct Record {
        std::array<unsigned char, salt_size> salt{};
        std::array<unsigned char, hash_size> hash{};
    };

    mutable std::mutex mutex_;
    std::map<std::string, Record, std::less<>> users_;
    std::array<unsigned char, salt_size> dummy_salt_{};
    std::array<unsigned char, hash_size> dummy_hash_{};

    static bool valid_username(std::string_view username) {
        if (username.empty() || username.size() > 64) {
            return false;
        }

        for (unsigned char c : username) {
            const bool allowed =
                (c >= 'a' && c <= 'z') ||
                (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') ||
                c == '_' || c == '-' || c == '.';
            if (!allowed) {
                return false;
            }
        }
        return true;
    }

    static bool valid_password(std::string_view password) {
        return password.size() >= min_password_length &&
               password.size() <= max_password_length &&
               password.size() <= static_cast<std::size_t>(INT_MAX);
    }

    static bool derive(
        std::string_view password,
        const std::array<unsigned char, salt_size>& salt,
        std::array<unsigned char, hash_size>& output) {
        const char* password_data = password.empty() ? "" : password.data();
        return PKCS5_PBKDF2_HMAC(
                   password_data,
                   static_cast<int>(password.size()),
                   salt.data(),
                   static_cast<int>(salt.size()),
                   iterations,
                   EVP_sha256(),
                   static_cast<int>(output.size()),
                   output.data()) == 1;
    }
};