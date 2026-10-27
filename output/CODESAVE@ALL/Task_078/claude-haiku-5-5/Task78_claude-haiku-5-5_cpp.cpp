```cpp
#include <openssl/rsa.h>
#include <openssl/bn.h>
#include <openssl/rand.h>
#include <openssl/err.h>

#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using Bytes = std::vector<uint8_t>;

static void put_u32(Bytes& b, uint32_t v) {
    b.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
    b.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
    b.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    b.push_back(static_cast<uint8_t>(v & 0xFF));
}

static void put_string(Bytes& b, const Bytes& data) {
    put_u32(b, static_cast<uint32_t>(data.size()));
    b.insert(b.end(), data.begin(), data.end());
}

static void put_cstring(Bytes& b, const std::string& s) {
    put_string(b, Bytes(s.begin(), s.end()));
}

static Bytes bn_to_mpint(const BIGNUM* bn) {
    Bytes out;
    if (BN_is_zero(bn)) {
        return out;
    }
    int len = BN_num_bytes(bn);
    Bytes raw(static_cast<size_t>(len));
    BN_bn2bin(bn, raw.data());
    if (raw[0] & 0x80) {
        out.push_back(0x00);
    }
    out.insert(out.end(), raw.begin(), raw.end());
    return out;
}

static void put_mpint(Bytes& b, const BIGNUM* bn) {
    put_string(b, bn_to_mpint(bn));
}

static std::string base64_encode(const Bytes& data) {
    static const char tbl[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    size_t i = 0;
    while (i + 2 < data.size()) {
        uint32_t v = (static_cast<uint32_t>(data[i]) << 16) |
                     (static_cast<uint32_t>(data[i + 1]) << 8) |
                     static_cast<uint32_t>(data[i + 2]);
        out.push_back(tbl[(v >> 18) & 0x3F]);
        out.push_back(tbl[(v >> 12) & 0x3F]);
        out.push_back(tbl[(v >> 6) & 0x3F]);
        out.push_back(tbl[v & 0x3F]);
        i += 3;
    }
    size_t rem = data.size() - i;
    if (rem == 1) {
        uint32_t v = static_cast<uint32_t>(data[i]) << 16;
        out.push_back(tbl[(v >> 18) & 0x3F]);
        out.push_back(tbl[(v >> 12) & 0x3F]);
        out += "==";
    } else if (rem == 2) {
        uint32_t v = (static_cast<uint32_t>(data[i]) << 16) |
                     (static_cast<uint32_t>(data[i + 1]) << 8);
        out.push_back(tbl[(v >> 18) & 0x3F]);
        out.push_back(tbl[(v >> 12) & 0x3F]);
        out.push_back(tbl[(v >> 6) & 0x3F]);
        out.push_back('=');
    }
    return out;
}

static std::string generate_openssh_rsa_private_key(int bits, const std::string& comment) {
    if (bits < 1024) {
        throw std::invalid_argument("RSA key size too small");
    }

    std::unique_ptr<RSA, decltype(&RSA_free)> rsa(RSA_new(), RSA_free);
    std::unique_ptr<BIGNUM, decltype(&BN_free)> e(BN_new(), BN_free);
    if (!rsa || !e) {
        throw std::runtime_error("allocation failure");
    }
    if (!BN_set_word(e.get(), 65537)) {
        throw std::runtime_error("failed to set public exponent");
    }
    if (RSA_generate_key_ex(rsa.get(), bits, e.get(), nullptr) != 1) {
        throw std::runtime_error("RSA key generation failed");
    }

    const BIGNUM *n = nullptr, *pub_e = nullptr, *d = nullptr;
    const BIGNUM *p = nullptr, *q = nullptr;
    const BIGNUM *dmp1 = nullptr, *dmq1 = nullptr, *iqmp = nullptr;
    RSA_get0_key(rsa.get(), &n, &pub_e, &d);
    RSA_get0_factors(rsa.get(), &p, &q);
    RSA_get0_crt_params(rsa.get(), &dmp1, &dmq1, &iqmp);

    const std::string keytype = "ssh-rsa";

    // Public key blob
    Bytes pubblob;
    put_cstring(pubblob, keytype);
    put_mpint(pubblob, pub_e);
    put_mpint(pubblob, n);

    // Private section (unencrypted)
    Bytes priv;
    uint8_t rnd[4];
    if (RAND_bytes(rnd, sizeof(rnd)) != 1) {
        throw std::runtime_error("RAND_bytes failed");
    }
    uint32_t checkint = (static_cast<uint32_t>(rnd[0]) << 24) |
                        (static_cast<uint32_t>(rnd[1]) << 16) |
                        (static_cast<uint32_t>(rnd[2]) << 8) |
                        static_cast<uint32_t>(rnd[3]);
    put_u32(priv, checkint);
    put_u32(priv, checkint);
    put_cstring(priv, keytype);
    put_mpint(priv, n);
    put_mpint(priv, pub_e);
    put_mpint(priv, d);
    put_mpint(priv, iqmp);
    put_mpint(priv, p);
    put_mpint(priv, q);
    put_cstring(priv, comment);

    // Pad to multiple of cipher block size (8 for "none")
    uint8_t padv = 1;
    while (priv.size() % 8 != 0) {
        priv.push_back(padv++);
    }

    // Assemble the openssh-key-v1 container
    Bytes blob;
    const std::string magic = "openssh