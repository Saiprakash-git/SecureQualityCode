#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <cstdint>
#include <stdexcept>
#include <algorithm>

class SecureVault {
private:
    std::map<std::string, std::vector<uint8_t>> storage;
    std::string filename;

    std::vector<uint8_t> encrypt(const std::string& data, const std::string& key) {
        std::vector<uint8_t> result(data.size());
        for (size_t i = 0; i < data.size(); ++i) {
            result[i] = static_cast<uint8_t>(data[i]) ^ static_cast<uint8_t>(key[i % key.size()]);
        }
        return result;
    }

    std::string decrypt(const std::vector<uint8_t>& data, const std::string& key) {
        std::string result(data.size(), '\0');
        for (size_t i = 0; i < data.size(); ++i) {
            result[i] = static_cast<char>(data[i] ^ static_cast<uint8_t>(key[i % key.size()]));
        }
        return result;
    }

public:
    SecureVault(const std::string& file) : filename(file) {
        loadFromFile();
    }

    void store(const std::string& id, const std::string& sensitiveData, const std::string& encryptionKey) {
        if (encryptionKey.empty()) {
            throw std::invalid_argument("Encryption key cannot be empty");
        }
        if (sensitiveData.empty()) {
            throw std::invalid_argument("Data cannot be empty");
        }
        storage[id] = encrypt(sensitiveData, encryptionKey);
        saveToFile();
    }

    std::string retrieve(const std::string& id, const std::string& encryptionKey) {
        auto it = storage.find(id);
        if (it == storage.end()) {
            throw std::runtime_error("Data not found for id: " + id);
        }
        return decrypt(it->second, encryptionKey);
    }

    bool contains(const std::string& id) const {
        return storage.find(id) != storage.end();
    }

    void remove(const std::string& id) {
        storage.erase(id);
        saveToFile();
    }

    size_t size() const {
        return storage.size();
    }

    void saveToFile() {
        std::ofstream file(filename, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Cannot open file for writing: " + filename);
        }

        size_t count = storage.size();
        file.write(reinterpret_cast<const char*>(&count), sizeof(count));

        for (const auto& pair : storage) {
            size_t idLen = pair.first.size();
            file