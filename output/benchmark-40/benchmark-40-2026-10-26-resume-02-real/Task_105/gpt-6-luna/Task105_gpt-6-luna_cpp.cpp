#include <arpa/inet.h>
#include <netdb.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>

using SslContextPtr = std::unique_ptr<SSL_CTX, decltype(&SSL_CTX_free)>;
using SslPtr = std::unique_ptr<SSL, decltype(&SSL_free)>;

int connectTcp(const std::string& host, const std::string& port) {
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    addrinfo* results = nullptr;
    const int lookupResult = getaddrinfo(host.c_str(), port.c_str(), &hints, &results);
    if (lookupResult != 0) {
        std::cerr << "Address lookup failed: " << gai_strerror(lookupResult) << '\n';
        return -1;
    }

    int socketFd = -1;
    for (addrinfo* address = results; address != nullptr; address = address->ai_next) {
        socketFd = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
        if (socketFd < 0) {
            continue;
        }

        if (connect(socketFd, address->ai_addr, address->ai_addrlen) == 0) {
            break;
        }

        close(socketFd);
        socketFd = -1;
    }

    freeaddrinfo(results);
    return socketFd;
}

bool isIpAddress(const std::string& host) {
    in_addr ipv4{};
    in6_addr ipv6{};
    return inet_pton(AF_INET, host.c_str(), &ipv4) == 1 ||
           inet_pton(AF_INET6, host.c_str(), &ipv6) == 1;
}

int establishTlsConnection(const std::string& host, const std::string& port) {
    if (OPENSSL_init_ssl(0, nullptr) != 1) {
        std::cerr << "OpenSSL initialization failed.\n";
        return 1;
    }

    SslContextPtr context(SSL_CTX_new(TLS_client_method()), SSL_CTX_free);
    if (!context) {
        ERR_print_errors_fp(stderr);
        return 1;
    }

    SSL_CTX_set_min_proto_version(context.get(), TLS1_2_VERSION);
    SSL_CTX_set_options(context.get(), SSL_OP_NO_COMPRESSION);
    SSL_CTX_set_verify(context.get(), SSL_VERIFY_PEER, nullptr);

    if (SSL_CTX_set_default_verify_paths(context.get()) != 1) {
        std::cerr << "Unable to load the system certificate trust store.\n";
        ERR_print_errors_fp(stderr);
        return 1;
    }

    const int socketFd = connectTcp(host, port);
    if (socketFd < 0) {
        std::cerr << "Unable to connect to " << host << ':' << port << ".\n";
        return 1;
    }

    SslPtr ssl(SSL_new(context.get()), SSL_free);
    if (!ssl) {
        ERR_print_errors_fp(stderr);
        close(socketFd);
        return 1;
    }

    if (SSL_set_fd(ssl.get(), socketFd) != 1) {
        std::cerr << "Unable to associate the socket with the TLS connection.\n";
        ERR_print_errors_fp(stderr);
        close(socketFd);
        return 1;
    }

    if (isIpAddress(host)) {
        X509_VERIFY_PARAM* parameters = SSL_get0_param(ssl.get());
        if (X509_VERIFY_PARAM_set1_ip_asc(parameters, host.c_str()) != 1) {
            std::cerr << "Unable to configure IP address certificate verification.\n";
            ERR_print_errors_fp(stderr);
            close(socketFd);
            return 1;
        }
    } else {
        if (SSL_set_tlsext_host_name(ssl.get(), host.c_str()) != 1 ||
            SSL_set1_host(ssl.get(), host.c_str()) != 1) {
            std::cerr << "Unable to configure server name or hostname verification.\n";
            ERR_print_errors_fp(stderr);
            close(socketFd);
            return 1;
        }
    }

    if (SSL_connect(ssl.get()) != 1) {
        std::cerr << "TLS handshake failed.\n";
        ERR_print_errors_fp(stderr);
        close(socketFd);
        return 1;
    }

    const long verificationResult = SSL_get_verify_result(ssl.get());
    if (verificationResult != X509_V_OK) {
        std::cerr << "Certificate verification failed: "
                  << X509_verify_cert_error_string(verificationResult) << '\n';
        close(socketFd);
        return 1;
    }

    const char* cipher = SSL_get_cipher_name(ssl.get());
    std::cout << "Secure TLS connection established to " << host << ':' << port
              << " using " << (cipher != nullptr ? cipher : "unknown cipher") << ".\n";

    SSL_shutdown(ssl.get());
    close(socketFd);
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <hostname-or-ip> <port>\n";
        return 2;
    }

    return establishTlsConnection(argv[1], argv[2]);
}