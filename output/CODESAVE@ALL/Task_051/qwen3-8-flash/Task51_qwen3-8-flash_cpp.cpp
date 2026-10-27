#include <iostream>
#include <string>

std::string encryptCaesar(const std::string& plaintext, int shift) {
    shift %= 26;
    if (shift < 0) {
        shift += 26;
    }

    std::string ciphertext;
    ciphertext.reserve(plaintext.size());

    for (char ch : plaintext) {
        if (ch >= 'A' && ch <= 'Z') {
            ciphertext.push_back(static_cast<char>('A' + (ch - 'A' + shift) % 26));
        } else if (ch >= 'a' && ch <= 'z') {
            ciphertext.push_back(static_cast<char>('a' + (ch - 'a' + shift) % 26));
        } else {
            ciphertext.push_back(ch);
        }
    }

    return ciphertext;
}

int main() {
    std::string plaintext;
    int shift = 3;

    std::getline(std::cin, plaintext);

    if (!(std::cin >> shift)) {
        shift = 3;
    }

    std::string ciphertext = encryptCaesar(plaintext, shift);
    std::cout << ciphertext << std::endl;

    return 0;
}