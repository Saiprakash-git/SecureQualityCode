#include <iostream>
#include <string>
#include <vector>
#include <cstdint>

std::string xorEncrypt(const std::string& text, const std::string& key) {
    std::string result = text;
    if (key.empty()) return result;
    for (size_t i = 0; i < text.size(); ++i) {
        result[i] = static_cast<char>(text[i] ^ key[i % key.size()]);
    }
    return result;
}

std::string toHex(const std::string& data) {
    static const char* digits = "0123456789abcdef";
    std::string out;
    out.reserve(data.size() * 2);
    for (unsigned char c : data) {
        out.push_back(digits[c >> 4]);
        out.push_back(digits[c & 0x0F]);
    }
    return out;
}

std::string hexToBytes(const std::string& hex) {
    std::string out;
    out.reserve(hex.size() / 2);
    auto val = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    for (size_t i = 0; i + 1 < hex.size(); i += 2) {
        int hi = val(hex[i]);
        int lo = val(hex[i + 1]);
        if (hi < 0 || lo < 0) break;
        out.push_back(static_cast<char>((hi << 4) | lo));
    }
    return out;
}

int main() {
    std::string text, key;
    std::cout << "Enter text to encrypt: ";
    std::getline(std::cin, text);
    std::cout << "Enter key: ";
    std::getline(std::cin, key);

    std::string cipher = xorEncrypt(text, key);
    std::string hexCipher = toHex(cipher);
    std::cout << "Encrypted (hex): " << hexCipher << "\n";

    std::string decrypted = xorEncrypt(hexToBytes(hexCipher), key);
    std::cout << "Decrypted: " << decrypted << "\n";

    return 0;
}