#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <iomanip>
#include <openssl/evp.h>
#include <openssl/aes.h>

static const unsigned char STATIC_IV[16] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
};

std::vector<unsigned char> padData(const std::vector<unsigned char>& data, int blockSize) {
    int padding = blockSize - (data.size() % blockSize);
    std::vector<unsigned char> padded = data;
    padded.insert(padded.end(), padding, static_cast<unsigned char>(padding));
    return padded;
}

std::vector<unsigned char> encryptAES_CBC(const std::vector<unsigned char>& plaintext,
                                          const std::vector<unsigned char>& key,