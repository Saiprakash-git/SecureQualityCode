#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include <random>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/bn.h>
#include <openssl/rand.h>
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
#include <openssl/core_names.h>
#endif

namespace {

void write_u32(std::vector<uint8_t>& buf, uint32_t val) {
    buf.push_back(static_cast<uint8_t>((val >> 2