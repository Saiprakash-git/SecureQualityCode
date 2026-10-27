#include <iostream>
#include <string>
#include <iomanip>

std::string encrypt(const std::string& plaintext, const std::string& key) {
    std::string result;
    for (size_t i = 0; i < plaintext.size(); ++i) {
        result.push_back(plaintext[i] ^ key[i % key.size()]);
    }
    return result;
}

std::string decrypt(const std::string& ciphertext, const std::string& key) {
    return encrypt(ciphertext, key);
}

int main() {
    std::string text, key;
    std::cout << "Enter string to encrypt: ";
    std::getline(std::cin, text);
    std::cout << "Enter encryption key: ";
    std::getline(std::cin, key);

    std::string encrypted = encrypt(text, key);
    std::string decrypted = decrypt(encrypted, key);

    std::cout << "Encrypted (hex): ";
    for (unsigned char c : encrypted) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)c;
    }
    std::cout << std::dec << std::endl;
    std::cout << "Decrypted: " << decrypted << std::endl;

    return 0;
}