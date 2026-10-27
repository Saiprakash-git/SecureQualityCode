#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <vector>

namespace {

using ByteVector = std::vector<unsigned char>;

void appendUint32(ByteVector& out, std::uint32_t value) {
    out.push_back(static_cast<unsigned char>((value >> 24) & 0xff));
    out.push_back(static_cast<unsigned char>((value >> 16) & 0xff));
    out.push_back(static_cast<unsigned char>((value >> 8) & 0xff));
    out.push_back(static_cast<unsigned char>(value & 0xff));
}

bool appendString(ByteVector& out, const unsigned char* data, std::size_t size) {
    if (size > std::numeric_limits<std::uint32_t>::max()) {
        return false;
    }
    appendUint32(out, static_cast<std::uint32_t>(size));
    out.insert(out.end(), data, data + size);
    return true;
}

bool appendString(ByteVector& out, const std::string& value) {
    return appendString(out,
                        reinterpret_cast<const unsigned char*>(value.data()),
                        value.size());
}

bool appendMpint(ByteVector& out, const BIGNUM* number) {
    if (number == nullptr || BN_is_negative(number)) {
        return false;
    }

    const int byteCount = BN_num_bytes(number);
    if (byteCount == 0) {
        return appendString(out, nullptr, 0);
    }

    ByteVector bytes(static_cast<std::size_t>(byteCount));
    if (BN_bn2bin(number, bytes.data()) != byteCount) {
        return false;
    }

    if ((bytes.front() & 0x80) != 0) {
        bytes.insert(bytes.begin(), 0);
    }
    return appendString(out, bytes.data(), bytes.size());
}

std::string armorOpenSSH(const ByteVector& data) {
    if (data.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        return {};
    }

    const std::size_t encodedCapacity = 4 * ((data.size() + 2) / 3);
    std::string encoded(encodedCapacity, '\0');
    const int encodedLength = EVP_EncodeBlock(
        reinterpret_cast<unsigned char*>(&encoded[0]),
        data.data(),
        static_cast<int>(data.size()));

    if (encodedLength < 0) {
        return {};
    }
    encoded.resize(static_cast<std::size_t>(encodedLength));

    std::string result = "-----BEGIN OPENSSH PRIVATE KEY-----\n";
    for (std::size_t offset = 0; offset < encoded.size(); offset += 70) {
        result.append(encoded, offset, std::min<std::size_t>(70, encoded.size() - offset));
        result.push_back('\n');
    }
    result += "-----END OPENSSH PRIVATE KEY-----\n";
    return result;
}

}  // namespace

std::string generateOpenSSHPrivateKey(int bits, const std::string& comment) {
    if (bits < 1024 || bits > 16384) {
        return {};
    }

    using PkeyContextPtr = std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)>;
    using PkeyPtr = std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>;
    using RsaPtr = std::unique_ptr<RSA, decltype(&RSA_free)>;

    PkeyContextPtr context(EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr),
                           EVP_PKEY_CTX_free);
    if (!context || EVP_PKEY_keygen_init(context.get()) <= 0 ||
        EVP_PKEY_CTX_set_rsa_keygen_bits(context.get(), bits) <= 0) {
        return {};
    }

    EVP_PKEY* rawKey = nullptr;
    if (EVP_PKEY_keygen(context.get(), &rawKey) <= 0 || rawKey == nullptr) {
        return {};
    }
    PkeyPtr key(rawKey, EVP_PKEY_free);

    RsaPtr rsa(EVP_PKEY_get1_RSA(key.get()), RSA_free);
    if (!rsa) {
        return {};
    }

    const BIGNUM *n = nullptr, *e = nullptr, *d = nullptr;
    const BIGNUM *p = nullptr, *q = nullptr, *dmp1 = nullptr;
    const BIGNUM *dmq1 = nullptr, *iqmp = nullptr;
    RSA_get0_key(rsa.get(), &n, &e, &d);
    RSA_get0_factors(rsa.get(), &p, &q);
    RSA_get0_crt_params(rsa.get(), &dmp1, &dmq1, &iqmp);
    if (n == nullptr || e == nullptr || d == nullptr || p == nullptr ||
        q == nullptr || iqmp == nullptr) {
        return {};
    }

    ByteVector publicKey;
    if (!appendString(publicKey, "ssh-rsa") ||
        !appendMpint(publicKey, e) ||
        !appendMpint(publicKey, n)) {
        return {};
    }

    ByteVector privateKey;
    unsigned char checkBytes[4];
    if (RAND_bytes(checkBytes, sizeof(checkBytes)) != 1) {
        return {};
    }
    const std::uint32_t check =
        (static_cast<std::uint32_t>(checkBytes[0]) << 24) |
        (static_cast<std::uint32_t>(checkBytes[1]) << 16) |
        (static_cast<std::uint32_t>(checkBytes[2]) << 8) |
        static_cast<std::uint32_t>(checkBytes[3]);

    appendUint32(privateKey, check);
    appendUint32(privateKey, check);

    if (!appendString(privateKey, "ssh-rsa") ||
        !appendMpint(privateKey, n) ||
        !appendMpint(privateKey, e) ||
        !appendMpint(privateKey, d) ||
        !appendMpint(privateKey, iqmp) ||
        !appendMpint(privateKey, p) ||
        !appendMpint(privateKey, q) ||
        !appendString(privateKey, comment)) {
        return {};
    }

    for (unsigned char padding = 1; privateKey.size() % 8 != 0; ++padding) {
        privateKey.push_back(padding);
    }

    ByteVector output;
    static const unsigned char magic[] = {
        'o', 'p', 'e', 'n', 's', 's', 'h', '-', 'k', 'e', 'y', '-', 'v', '1', 0
    };
    output.insert(output.end(), magic, magic + sizeof(magic));

    if (!appendString(output, "none") ||
        !appendString(output, "none") ||
        !appendString(output, "") ||
        (appendUint32(output, 1), !appendString(output, publicKey.data(), publicKey.size())) ||
        !appendString(output, privateKey.data(), privateKey.size())) {
        return {};
    }

    return armorOpenSSH(output);
}

int main(int argc, char* argv[]) {
    int bits = 3072;
    std::string comment = "generated-by-cpp";

    if (argc > 1) {
        char* end = nullptr;
        const long parsed = std::strtol(argv[1], &end, 10);
        if (end == argv[1] || *end != '\0' ||
            parsed < 1024 || parsed > 16384) {
            return 1;
        }
        bits = static_cast<int>(parsed);
    }
    if (argc > 2) {
        comment = argv[2];
    }

    const std::string key = generateOpenSSHPrivateKey(bits, comment);
    if (key.empty()) {
        return 1;
    }

    std::cout << key;
    return std::cout ? 0 : 1;
}