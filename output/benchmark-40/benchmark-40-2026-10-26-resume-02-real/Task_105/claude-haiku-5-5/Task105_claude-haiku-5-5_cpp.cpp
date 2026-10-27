#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/crypto.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

struct TlsResult {
    bool ok = false;
    std::string error;
    std::string tlsVersion;
    std::string cipher;
    std::string subject;
    std::string issuer;
    std::string response;
};

static std::string collectOpenSslErrors() {
    std::ostringstream oss;
    unsigned long code;
    while ((code = ERR_get_error()) != 0) {
        char buf[256];
        ERR_error_string_n(code, buf, sizeof(buf));
        if (oss.tellp() > 0) oss << "; ";
        oss << buf;
    }
    return oss.str();
}

static int tcpConnect(const std::string& host, const std::string& port,
                      int timeoutSeconds, std::string& err) {
    struct addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    struct addrinfo* res = nullptr;
    int rc = getaddrinfo(host.c_str(), port.c_str(), &hints, &res);
    if (rc != 0) {
        err = std::string("getaddrinfo failed: ") + gai_strerror(rc);
        return -1;
    }

    int fd = -1;
    for (struct addrinfo* ai = res; ai != nullptr; ai = ai->ai_next) {
        fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (fd < 0) continue;

        struct timeval tv;
        tv.tv_sec = timeoutSeconds;
        tv.tv_usec = 0;
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

        if (connect(fd, ai->ai_addr, ai->ai_addrlen) == 0) {
            break;
        }
        close(fd);
        fd = -1;
    }
    freeaddrinfo(res);

    if (fd < 0) {
        err = "unable to connect to " + host + ":" + port;
    }
    return fd;
}

static SSL_CTX* createClientContext(std::string& err) {
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx) {
        err = "SSL_CTX_new failed: " + collectOpenSslErrors();
        return nullptr;
    }

    if (SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION) != 1) {
        err = "failed to set minimum TLS version: " + collectOpenSslErrors();
        SSL_CTX_free(ctx);
        return nullptr;
    }

    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, nullptr);

    if (SSL_CTX_set_default_verify_paths(ctx) != 1) {
        err = "failed to load default CA paths: " + collectOpenSslErrors();
        SSL_CTX_free(ctx);
        return nullptr;
    }

    SSL_CTX_set_options(ctx, SSL_OP_NO_COMPRESSION);
    return ctx;
}

static bool readAll(SSL* ssl, std::string& out, size_t maxBytes, std::string& err) {
    char buf[4096];
    while (out.size() < maxBytes) {
        int n = SSL_read(ssl, buf, sizeof(buf));
        if (n > 0) {
            out.append(buf, static_cast<size_t>(n));
            continue;
        }
        int reason = SSL_get_error(ssl, n);
        if (reason == SSL_ERROR_ZERO_RETURN) {
            return true;
        }
        if (reason == SSL_ERROR_SYSCALL && n == 0) {
            return true;
        }
        err = "SSL_read failed: " + collectOpenSslErrors();
        return false;
    }
    return true;
}

TlsResult establishSecureConnection(const std::string& host,
                                    const std::string& port,
                                    const std::string& path,
                                    int timeoutSeconds = 10,
                                    size_t maxResponseBytes = 64 * 1024) {
    TlsResult result;

    if (host.empty() || port.empty()) {
        result.error = "host and port must not be empty";
        return result;
    }

    ERR_clear_error();

    std::string err;
    SSL_CTX* ctx = createClientContext(err);
    if (!ctx) {
        result.error = err;
        return result;
    }

    int fd = tcpConnect(host, port, timeoutSeconds, err);
    if (fd < 0) {
        result.error = err;
        SSL_CTX_free(ctx);
        return result;
    }

    SSL* ssl = SSL_new(ctx);
    if (!ssl) {
        result.error = "SSL_new failed: " + collectOpenSslErrors();
        close(fd);
        SSL_CTX_free(ctx);
        return result;
    }

    SSL_set_fd(ssl, fd);

    // Server Name Indication
    if (SSL_set_tlsext_host_name(ssl, host.c_str()) != 1) {
        result.error = "failed to set SNI: " + collectOpenSslErrors();
        SSL_free(ssl);
        close(fd);
        SSL_CTX_free(ctx);
        return result;
    }

    // Hostname verification against certificate
    X509_VERIFY_PARAM* param = SSL_get0_param(ssl);
    X509_VERIFY_PARAM_set_hostflags(param, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
    if (SSL_set1_host(ssl, host.c_str()) != 1) {
        result.error = "failed to set expected hostname: " + collectOpenSslErrors();
        SSL_free(ssl);
        close(fd);
        SSL_CTX_free(ctx);
        return result;
    }

    if (SSL_connect(ssl) != 1) {
        result.error = "TLS handshake failed: " + collectOpenSslErrors();
        SSL_free(ssl);
        close(fd);
        SSL_CTX_free(ctx);
        return result;
    }

    long verify = SSL_get_verify_result(ssl);
    if (verify != X509_V_OK) {
        result.error = std::string("certificate verification failed: ") +
                       X509_verify_cert_error_string(verify);
        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(fd);
        SSL_CTX_free(ctx);
        return result;
    }

    result.tlsVersion = SSL_get_version(ssl);
    const char* cipher = SSL_get_cipher(ssl);
    result.cipher = cipher ? cipher : "";

    X509* cert = SSL_get0_peer_certificate(ssl);
    if (cert) {
        char* subj = X509_NAME_oneline(X509_get_subject_name(cert), nullptr, 0);
        char* iss = X509_NAME_oneline(X509_get_issuer_name(cert), nullptr, 0);
        if (subj) { result.subject = subj; OPENSSL_free(subj); }
        if (iss) { result.issuer = iss; OPENSSL_free(iss); }
    }

    std::string request = "GET " + (path.empty() ? std::string("/") : path) +
                          " HTTP/1.1\r\n"
                          "Host: " + host + "\r\n"
                          "User-Agent: secure-tls-client/1.0\r\n"
                          "Accept: */*\r\n"
                          "Connection: close\r\n\