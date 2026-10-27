#include <iostream>
#include <vector>
#include <cstdint>
#include <iomanip>
#include <openssl/evp.h>

static const unsigned char STATIC_IV[16] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};

std::vector<uint8_t> pad_data_pkcs7(const std::vector<uint8_t>& input, size_t block_size) {
    size_t padding_len = block_size - (input.size() % block_size);
    std::vector<uint8_t> padded = input;
    padded.insert(padded.end(), padding_len, static_cast<uint8_t>(padding_len));
    return padded;
}

std::vector<uint8_t> encrypt_aes_cbc(const std::vector<uint8_t>& plaintext, const std::vector<uint8_t>& key) {
    const size_t block_size = 16;
    
    std::vector<uint8_t> padded_data = pad_data_pkcs7(plaintext, block_size);

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return {};
    }

    const EVP_CIPHER* cipher = (key.size() == 32) ? EVP_aes_256_cbc() : EVP_aes_128_cbc();

    if (EVP_EncryptInit_ex(ctx, cipher, nullptr, key.data(), STATIC_IV) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }

    EVP_CIPHER_CTX_set_padding(ctx, 0);

    std::vector<uint8_t> ciphertext(padded_data.size() + block_size);
    int len = 0;
    int total_len = 0;

    if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len, padded_data.data(), static_cast<int>(padded_data.size())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }
    total_len = len;

    if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }
    total_len += len;

    EVP_CIPHER_CTX_free(ctx);

    ciphertext.resize(total_len);
    return ciphertext;
}

int main() {
    std::vector<uint8_t> key = {
        0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
        0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
    };

    std::string text = "Hello, AES CBC Encryption!";
    std::vector<uint8_t> plaintext(text.begin(), text.end());

    std::vector<uint8_t> encrypted = encrypt_aes_cbc(plaintext, key);

    for (uint8_t byte : encrypted) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }
    std::cout << std::endl;

    return 0;
}