#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>
#include <functional>

// Demo-level protection only. The XOR keystream below is NOT a vetted cipher.
// For production, replace encryptData/decryptData with a standard library such as OpenSSL or libsodium.

struct Record {
    int id;
    std::string name;
    std::string email;
    std::string card;
    std::string expiry;
};

std::string stripSpaces(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c != ' ' && c != '-') out += c;
    }
    return out;
}

bool luhnValid(const std::string& digits) {
    int sum = 0;
    bool doubleIt = false;
    for (int i = static_cast<int>(digits.size()) - 1; i >= 0; --i) {
        int d = digits[i] - '0';
        if (doubleIt) {
            d *= 2;
            if (d > 9) d -= 9;
        }
        sum += d;
        doubleIt = !doubleIt;
    }
    return sum % 10 == 0;
}

bool isValidCard(const std::string& raw) {
    std::string digits = stripSpaces(raw);
    if (digits.size() < 13 || digits.size() > 19) return false;
    for (char c : digits) {
        if (!std::isdigit(static_cast<unsigned char>(c))) return false;
    }
    return luhnValid(digits);
}

std::string maskCard(const std::string& raw) {
    std::string digits = stripSpaces(raw);
    if (digits.size() <= 4) return std::string(digits.size(), '*');
    return std::string(digits.size() - 4, '*') + digits.substr(digits.size() - 4);
}

bool isValidExpiry(const std::string& s) {
    if (s.size() != 5 || s[2] != '/') return false;
    if (!std::isdigit(static_cast<unsigned char>(s[0])) || !std::isdigit(static_cast<unsigned char>(s[1])) ||
        !std::isdigit(static_cast<unsigned char>(s[3])) || !std::isdigit(static_cast<unsigned char>(s[4])))
        return false;
    int month = (s[0] - '0') * 10 + (s[1] - '0');
    return month >= 1 && month <= 12;
}

bool isSafeField(const std::string& s) {
    if (s.empty()) return false;
    for (char c : s) {
        if (c == '|' || c == '\n' || c == '\r') return false;
    }
    return true;
}

std::string serialize(const std::vector<Record>& records) {
    std::ostringstream out;
    for (const auto& r : records) {
        out << r.id << '|' << r.name << '|' << r.email << '|' << r.card << '|' << r.expiry << '\n';
    }
    return out.str();
}

std::vector<Record> deserialize(const std::string& text) {
    std::vector<Record> records;
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        std::vector<std::string> parts;
        std::string part;
        std::istringstream ls(line);
        while (std::getline(ls, part, '|')) parts.push_back(part);
        if (parts.size() != 5) continue;
        Record r;
        try {
            r.id = std::stoi(parts[0]);
        } catch (...) {
            continue;
        }
        r.name = parts[1];
        r.email = parts[2];
        r.card = parts[3];
        r.expiry = parts[4];
        records.push_back(r);
    }
    return records;
}

std::string xorStream(const std::string& data, const std::string& key) {
    std::seed_seq seq(key.begin(), key.end());
    std::mt19937_64 gen(std::hash<std::string>{}(key));
    std::string out(data.size(), '\0');
    for (size_t i = 0; i < data.size(); ++i) {
        out[i] = static_cast<char>(data[i] ^ static_cast<char>(gen() & 0xFF));
    }
    return out;
}

std::string toHex(const std::string& data) {
    static const char* hex = "0123456789abcdef";
    std::string out;
    out.reserve(data.size() * 2);
    for (unsigned char c : data) {
        out += hex[c >> 4];
        out += hex[c & 0x0F];
    }
    return out;
}

bool fromHex(const std::string& hex, std::string& out) {
    if (hex.size() % 2 != 0) return false;
    out.clear();
    out.reserve(hex.size() / 2);
    for (size_t i = 0; i < hex.size(); i += 2) {
        auto val = [](char c) -> int {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            return -1;
        };
        int hi = val(hex[i]), lo = val(hex[i + 1]);
        if (hi < 0 || lo < 0) return false;
        out += static_cast<char>((hi << 4) | lo);
    }
    return true;
}

std::string encryptData(const std::string& plain, const std::string& key) {
    return toHex(xorStream(plain, key));
}

bool decryptData(const std::string& cipherHex, const std::string& key, std::string& plain) {
    std::string raw;
    if (!fromHex(cipherHex, raw)) return false;
    plain = xorStream(raw, key);
    return true;
}

bool saveRecords(const std::string& path, const std::vector<Record>& records, const std::string& key) {
    std::ofstream f(path, std::ios::trunc);
    if (!f) return false;
    f << encryptData(serialize(records), key) << '\n';
    return f.good();
}

bool loadRecords(const std::string& path, const std::string& key, std::vector<Record>& records) {
    std::ifstream f(path);
    if (!f) return false;
    std::string cipherHex;
    std::getline(f, cipherHex);
    std::string plain;
    if (!decryptData(cipherHex, key, plain)) return false;
    records = deserialize(plain);
    return true;
}

int nextId(const std::vector<Record>& records) {
    int maxId = 0;
    for (const auto& r : records) maxId = std::max(maxId, r.id);
    return maxId + 1;
}

bool addRecord(std::vector<Record>& records, const Record& r) {
    if (!isSafeField(r.name) || !isSafeField(r.email) || !isValidCard(r.card) || !isValidExpiry(r.expiry))
        return false;
    Record copy = r;
    copy.id = nextId(records);
    copy.card = stripSpaces(r.card);
    records.push_back(copy);
    return true;
}

bool deleteRecord(std::vector<Record>& records, int id) {
    auto it = std::remove_if(records.begin(), records.end(), [id](const Record& r) { return r.id == id; });
    bool found = it != records.end();
    records.erase(it, records.end());
    return found;
}

std::vector<Record> searchByName(const std::vector<Record>& records, const std::string& term) {
    std::vector<Record> result;
    std::string lowerTerm = term;
    std::transform(lowerTerm.begin(), lowerTerm.end(), lowerTerm.begin(), ::tolower);
    for (const auto& r : records) {
        std::string lowerName = r.name;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        if (lowerName.find(lowerTerm) != std::string::npos) result.push_back(r);
    }
    return result;
}

std::string formatRecord(const Record& r) {
    std::ostringstream out;
    out << "ID: " << r.id << "\n"
        << "  Name:   " << r.name << "\n"
        << "  Email:  " << r.email << "\n"
        << "  Card:   " << maskCard(r.card) << "\n"
        << "  Expiry: " << r.expiry << "\n";
    return out.str();
}

std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string s;
    std::getline(std::cin, s);
    return s;
}

int main() {
    std::string key = readLine("Enter master passphrase: ");
    if (key.empty()) {
        std::cerr << "Passphrase required.\n";
        return 1;
    }
    const std::string path = "vault.dat";
    std::vector<Record> records;

    while (true) {
        std::cout << "\n1) Add  2) List  3) Search  4) Delete  5) Save  6) Load  0) Quit\n";
        std::string choice = readLine("Choice: ");

        if (choice == "1") {