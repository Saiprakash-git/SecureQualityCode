#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#if defined(__linux__)
#include <cerrno>
#include <sys/random.h>
#include <fcntl.h>
#include <unistd.h>
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
#include <stdlib.h>
#else
#include <random>
#endif

std::vector<std::uint8_t> secureRandomBytes(std::size_t count) {
    std::vector<std::uint8_t> bytes(count);

#if defined(__linux__)
    std::size_t offset = 0;

    while (offset < count) {
        const ssize_t result = getrandom(bytes.data() + offset, count - offset, 0);
        if (result > 0) {
            offset += static_cast<std::size_t>(result);
        } else if (result < 0 && errno == EINTR) {
            continue;
        } else if (result < 0 && errno == ENOSYS) {
            break;
        } else {
            throw std::runtime_error("Unable to obtain secure random bytes");
        }
    }

    if (offset < count) {
        const int fd = open("/dev/urandom", O_RDONLY);
        if (fd < 0) {
            throw std::runtime_error("Unable to obtain secure random bytes");
        }

        while (offset < count) {
            const ssize_t result = read(fd, bytes.data() + offset, count - offset);
            if (result > 0) {
                offset += static_cast<std::size_t>(result);
            } else if (result < 0 && errno == EINTR) {
                continue;
            } else {
                close(fd);
                throw std::runtime_error("Unable to obtain secure random bytes");
            }
        }

        close(fd);
    }
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
    arc4random_buf(bytes.data(), bytes.size());
#else
    std::random_device source;
    for (std::size_t i = 0; i < count; ++i) {
        bytes[i] = static_cast<std::uint8_t>(source());
    }
#endif

    return bytes;
}

std::string generateToken(std::size_t byteCount) {
    if (byteCount == 0) {
        throw std::invalid_argument("Token size must be greater than zero");
    }

    const std::vector<std::uint8_t> bytes = secureRandomBytes(byteCount);
    static constexpr char hex[] = "0123456789abcdef";

    std::string token;
    token.reserve(byteCount * 2);
    for (const std::uint8_t byte : bytes) {
        token.push_back(hex[byte >> 4]);
        token.push_back(hex[byte & 0x0f]);
    }

    return token;
}

int main() {
    try {
        std::cout << generateToken(32) << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}