#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/err.h>
#include <openssl/encoder.h>
#include <string>
#include <memory>
#include <iostream>
#include <stdexcept>
#include <cstddef>

std::string generate_rsa_private_key_openssh(size_t bits) {
    if (bits < 512) {
        throw std::invalid_argument("Key size too small");
    }

    EVP_PKEY* pkey = EVP_PKEY_Q_keygen(NULL, NULL, "RSA", bits);
    if (!pkey) {
        throw std::runtime_error("Failed to generate RSA key");
    }

    std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> key(pkey, EVP_PKEY_free);

    BIO* bio = BIO_new(BIO_s_mem());
    if (!bio) {
        throw std::runtime_error("Failed to create BIO");
    }
    std::unique_ptr<BIO, decltype(&BIO_free)> mem(bio, BIO_free);

    OSSL_ENCODER_CTX* ectx = OSSL_ENCODER_CTX_new_for_pkey(
        key.get(),
        OSSL_KEYMGMT_SELECT_PRIVATE_KEY,
        "PEM",
        "openssh",
        NULL
    );
    if (!ectx) {
        throw std::runtime_error("Failed to create encoder context");
    }
    std::unique_ptr<OSSL_ENCODER_CTX, decltype(&OSSL_ENCODER_CTX_free)> encoder(ectx, OSSL_ENCODER_CTX_free);

    if (OSSL_ENCODER_to_bio(encoder.get(), mem.get()) != 1) {
        throw std::runtime_error("Failed to encode key to OpenSSH format");
    }

    BUF_MEM* bptr = NULL;
    BIO_get_mem_ptr(mem.get(), &bptr);
    if (!bptr) {
        throw std::runtime_error("Failed to get BIO data");
    }

    return std::string(bptr->data, bptr->length);
}

int main(int argc, char* argv[]) {
    try {
        size_t bits = 2048;
        if (argc > 1) {
            bits = static_cast<size_t>(std::stoul(argv[1]));
        }

        std::string private_key = generate_rsa_private_key_openssh(bits);
        std::cout << private_key;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}