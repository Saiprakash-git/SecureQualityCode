#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <vector>

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace {

constexpr std::array<unsigned char, 8> kMagic = {'S', 'V', 'L', 'T', '0', '0', '0', '1'};
constexpr std::size_t kSaltSize = 16;
constexpr std::size_t kNonceSize = 12;
constexpr std::size_t kTagSize = 16;
constexpr std::size_t kHeaderSize = kMagic.size() + kSaltSize + kNonceSize + 8;
constexpr std::size_t kMaxFileSize = 128 * 1024 * 1024;
constexpr std::size_t kMaxPlaintextSize = 64 * 1024 * 1024;
constexpr int kPbkdf2Iterations = 600000;

using Records = std::map<std::string, std::string>;

void putU32(std::vector<unsigned char>& out, std::uint32_t value) {
    out.push_back(static_cast<unsigned char>((value >> 24) & 0xff));
    out.push_back(static_cast<unsigned char>((value >> 16) & 0xff));
    out.push_back(static_cast<unsigned char>((value >> 8) & 0xff));
    out.push_back(static_cast<unsigned char>(value & 0xff));
}

void putU64(std::vector<unsigned char>& out, std::uint64_t value) {
    for (int shift = 56; shift >= 0; shift -= 8)
        out.push_back(static_cast<unsigned char>((value >> shift) & 0xff));
}

bool getU32(const std::vector<unsigned char>& data, std::size_t& pos, std::uint32_t& value) {
    if (pos > data.size() || data.size() - pos < 4)
        return false;
    value = (static_cast<std::uint32_t>(data[pos]) << 24) |
            (static_cast<std::uint32_t>(data[pos + 1]) << 16) |
            (static_cast<std::uint32_t>(data[pos + 2]) << 8) |
            static_cast<std::uint32_t>(data[pos + 3]);
    pos += 4;
    return true;
}

bool getU64(const std::vector<unsigned char>& data, std::size_t& pos, std::uint64_t& value) {
    if (pos > data.size() || data.size() - pos < 8)
        return false;
    value = 0;
    for (int i = 0; i < 8; ++i)
        value = (value << 8) | data[pos++];
    return true;
}

bool serializeRecords(const Records& records, std::vector<unsigned char>& out) {
    if (records.size() > std::numeric_limits<std::uint32_t>::max())
        return false;

    out.clear();
    putU32(out, static_cast<std::uint32_t>(records.size()));

    for (const auto& entry : records) {
        if (entry.first.size() > std::numeric_limits<std::uint32_t>::max() ||
            entry.second.size() > std::numeric_limits<std::uint32_t>::max())
            return false;

        putU32(out, static_cast<std::uint32_t>(entry.first.size()));
        putU32(out, static_cast<std::uint32_t>(entry.second.size()));
        out.insert(out.end(), entry.first.begin(), entry.first.end());
        out.insert(out.end(), entry.second.begin(), entry.second.end());

        if (out.size() > kMaxPlaintextSize)
            return false;
    }
    return true;
}

bool deserializeRecords(const std::vector<unsigned char>& data, Records& records) {
    std::size_t pos = 0;
    std::uint32_t count = 0;
    if (!getU32(data, pos, count) || count > 1000000)
        return false;

    Records parsed;
    for (std::uint32_t i = 0; i < count; ++i) {
        std::uint32_t keySize = 0;
        std::uint32_t valueSize = 0;
        if (!getU32(data, pos, keySize) || !getU32(data, pos, valueSize))
            return false;
        if (pos > data.size() || keySize > data.size() - pos)
            return false;

        std::string key(reinterpret_cast<const char*>(data.data() + pos), keySize);
        pos += keySize;

        if (pos > data.size() || valueSize > data.size() - pos)
            return false;
        std::string value(reinterpret_cast<const char*>(data.data() + pos), valueSize);
        pos += valueSize;

        if (key.empty() || !parsed.emplace(std::move(key), std::move(value)).second)
            return false;
    }

    if (pos != data.size())
        return false;
    records = std::move(parsed);
    return true;
}

bool deriveKey(const std::string& password,
               const unsigned char* salt,
               std::array<unsigned char, 32>& key) {
    return PKCS5_PBKDF2_HMAC(password.data(),
                              static_cast<int>(password.size()),
                              salt,
                              static_cast<int>(kSaltSize),
                              kPbkdf2Iterations,
                              EVP_sha256(),
                              static_cast<int>(key.size()),
                              key.data()) == 1;
}

bool encryptData(const std::vector<unsigned char>& plaintext,
                 const std::string& password,
                 std::vector<unsigned char>& fileData) {
    if (plaintext.size() > kMaxPlaintextSize ||
        plaintext.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        return false;

    std::array<unsigned char, kSaltSize> salt{};
    std::array<unsigned char, kNonceSize> nonce{};
    std::array<unsigned char, 32> key{};

    if (RAND_bytes(salt.data(), static_cast<int>(salt.size())) != 1 ||
        RAND_bytes(nonce.data(), static_cast<int>(nonce.size())) != 1 ||
        !deriveKey(password, salt.data(), key)) {
        OPENSSL_cleanse(key.data(), key.size());
        return false;
    }

    std::vector<unsigned char> header;
    header.insert(header.end(), kMagic.begin(), kMagic.end());
    header.insert(header.end(), salt.begin(), salt.end());
    header.insert(header.end(), nonce.begin(), nonce.end());
    putU64(header, static_cast<std::uint64_t>(plaintext.size()));

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        OPENSSL_cleanse(key.data(), key.size());
        return false;
    }

    std::vector<unsigned char> ciphertext(plaintext.size() + 16);
    std::array<unsigned char, kTagSize> tag{};
    int outLength = 0;
    int finalLength = 0;
    bool ok =
        EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) == 1 &&
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN,
                            static_cast<int>(nonce.size()), nullptr) == 1 &&
        EVP_EncryptInit_ex(ctx, nullptr, nullptr, key.data(), nonce.data()) == 1 &&
        EVP_EncryptUpdate(ctx, nullptr, &outLength, header.data(),
                          static_cast<int>(header.size())) == 1 &&
        EVP_EncryptUpdate(ctx, ciphertext.data(), &outLength, plaintext.data(),
                          static_cast<int>(plaintext.size())) == 1 &&
        EVP_EncryptFinal_ex(ctx, ciphertext.data() + outLength, &finalLength) == 1 &&
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG,
                            static_cast<int>(tag.size()), tag.data()) == 1;

    EVP_CIPHER_CTX_free(ctx);
    OPENSSL_cleanse(key.data(), key.size());

    if (!ok)
        return false;

    ciphertext.resize(static_cast<std::size_t>(outLength + finalLength));
    fileData = std::move(header);
    fileData.insert(fileData.end(), ciphertext.begin(), ciphertext.end());
    fileData.insert(fileData.end(), tag.begin(), tag.end());
    return true;
}

bool decryptData(const std::vector<unsigned char>& fileData,
                 const std::string& password,
                 std::vector<unsigned char>& plaintext) {
    if (fileData.size() < kHeaderSize + kTagSize || fileData.size() > kMaxFileSize)
        return false;
    if (!std::equal(kMagic.begin(), kMagic.end(), fileData.begin()))
        return false;

    std::size_t pos = kMagic.size();
    const unsigned char* salt = fileData.data() + pos;
    pos += kSaltSize;
    const unsigned char* nonce = fileData.data() + pos;
    pos += kNonceSize;

    std::uint64_t declaredSize = 0;
    if (!getU64(fileData, pos, declaredSize) || declaredSize > kMaxPlaintextSize)
        return false;
    if (declaredSize != fileData.size() - kHeaderSize - kTagSize)
        return false;

    const unsigned char* ciphertext = fileData.data() + kHeaderSize;
    const unsigned char* tag = fileData.data() + fileData.size() - kTagSize;
    std::array<unsigned char, 32> key{};
    if (!deriveKey(password, salt, key)) {
        OPENSSL_cleanse(key.data(), key.size());
        return false;
    }

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        OPENSSL_cleanse(key.data(), key.size());
        return false;
    }

    plaintext.assign(static_cast<std::size_t>(declaredSize) + 16, 0);
    int outLength = 0;
    int finalLength = 0;
    bool ok =
        EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) == 1 &&
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN,
                            static_cast<int>(kNonceSize), nullptr) == 1 &&
        EVP_DecryptInit_ex(ctx, nullptr, nullptr, key.data(), nonce) == 1 &&
        EVP_DecryptUpdate(ctx, nullptr, &outLength, fileData.data(),
                          static_cast<int>(kHeaderSize)) == 1 &&
        EVP_DecryptUpdate(ctx, plaintext.data(), &outLength, ciphertext,
                          static_cast<int>(declaredSize)) == 1 &&
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG,
                            static_cast<int>(kTagSize), const_cast<unsigned char*>(tag)) == 1 &&
        EVP_DecryptFinal_ex(ctx, plaintext.data() + outLength, &finalLength) == 1;

    EVP_CIPHER_CTX_free(ctx);
    OPENSSL_cleanse(key.data(), key.size());

    if (!ok) {
        OPENSSL_cleanse(plaintext.data(), plaintext.size());
        plaintext.clear();
        return false;
    }

    plaintext.resize(static_cast<std::size_t>(outLength + finalLength));
    return plaintext.size() == declaredSize;
}

bool readFile(const std::filesystem::path& path, std::vector<unsigned char>& contents) {
#ifdef _WIN32
    std::error_code ec;
    const auto size = std::filesystem::file_size(path, ec);
    if (ec || size > kMaxFileSize)
        return false;
    std::ifstream file(path, std::ios::binary);
    if (!file)
        return false;
    contents.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    return file.good() || file.eof();
#else
    int fd = open(path.c_str(), O_RDONLY
#ifdef