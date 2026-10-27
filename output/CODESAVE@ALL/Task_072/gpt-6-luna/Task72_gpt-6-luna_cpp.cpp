#include <array>
#include <climits>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <openssl/evp.h>

static const std::array<unsigned char, 16> IV = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
};

std::vector<unsigned char> decodeHex(const std::string& hex) {
    if (hex.size() % 2 != 0) {
        throw std::invalid_argument("Hexadecimal key must have an even number of characters");
    }

    auto nibble = [](char c) -> unsigned char {
        if (c >= '0' && c <= '9') return static_cast<unsigned char>(c - '0');
        if (c >= 'a' && c <= 'f') return static_cast<unsigned char>(c - 'a' + 10);
        if (c >= 'A' && c <= 'F') return static_cast<unsigned char>(c - 'A' + 10);
        throw std::invalid_argument("Invalid hexadecimal key");
    };

    std::vector<unsigned char> result;
    result.reserve(hex.size() / 2);
    for (std::size_t i = 0; i < hex.size(); i += 2) {
        result.push_back(static_cast<unsigned char>((nibble(hex[i]) << 4) | nibble(hex[i + 1])));
    }
    return result;
}

std::vector<unsigned char> encryptAes256Cbc(
    const std::vector<unsigned char>& plaintext,
    const std::vector<unsigned char>& key
) {
    if (key.size() != 32) {
        throw std::invalid_argument("AES-256 requires a 32-byte key");
    }
    if (plaintext.size() > static_cast<std::size_t>(INT_MAX) - 16) {
        throw std::length_error("Input is too large");
    }

    constexpr std::size_t blockSize = 16;
    const unsigned char padding = static_cast<unsigned char>(
        blockSize - (plaintext.size() % blockSize)
    );

    std::vector<unsigned char> padded = plaintext;
    padded.insert(padded.end(), padding, padding);

    std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)> context(
        EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free
    );
    if (!context) {
        throw std::runtime_error("Failed to create cipher context");
    }

    if (EVP_EncryptInit_ex(
            context.get(), EVP_aes_256_cbc(), nullptr, key.data(), IV.data()
        ) != 1 ||
        EVP_CIPHER_CTX_set_padding(context.get(), 0) != 1) {
        throw std::runtime_error("Failed to initialize AES-CBC encryption");
    }

    std::vector<unsigned char> ciphertext(padded.size() + blockSize);
    int outputLength = 0;
    int finalLength = 0;

    if (EVP_EncryptUpdate(
            context.get(),
            ciphertext.data(),
            &outputLength,
            padded.data(),
            static_cast<int>(padded.size())
        ) != 1 ||
        EVP_EncryptFinal_ex(
            context.get(),
            ciphertext.data() + outputLength,
            &finalLength
        ) != 1) {
        throw std::runtime_error("AES-CBC encryption failed");
    }

    ciphertext.resize(static_cast<std::size_t>(outputLength + finalLength));
    return ciphertext;
}

int main(int argc, char* argv[]) {
    try {
        if (argc != 2) {
            std::cerr << "Usage: " << argv[0] << " <64-character-hex-key>\n";
            return 1;
        }

        const std::vector<unsigned char> key = decodeHex(argv[1]);
        const std::vector<unsigned char> plaintext(
            std::istreambuf_iterator<char>(std::cin),
            std::istreambuf_iterator<char>()
        );

        const std::vector<unsigned char> ciphertext =
            encryptAes256Cbc(plaintext, key);

        std::cout.write(
            reinterpret_cast<const char*>(ciphertext.data()),
            static_cast<std::streamsize>(ciphertext.size())
        );
        return std::cout ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}