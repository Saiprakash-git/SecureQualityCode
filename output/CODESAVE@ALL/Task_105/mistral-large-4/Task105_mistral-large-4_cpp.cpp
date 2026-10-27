#include <iostream>
#include <string>
#include <cstring>
#include <stdexcept>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

struct SSLConnection {
    SSL_CTX* ctx;
    SSL* ssl;
    int sockfd;
};

bool initializeSSL();
SSLConnection* createSSLConnection(const std::string& hostname, int port);
bool sendSSLData(SSLConnection* conn, const std::string& data);
std::string receiveSSLData(SSLConnection* conn, int maxBytes);
void closeSSLConnection(SSLConnection* conn);
void cleanupSSL();

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <hostname> <port>" << std::endl;
        return 1;
    }

    std::string hostname = argv[1];
    int port = std::stoi(argv[2]);

    if (!initializeSSL()) {
        return 1;
    }

    SSLConnection* conn = createSSLConnection(hostname, port);
    if (!conn) {
        cleanupSSL();
        return 1;
    }

    std::string request = "GET / HTTP/1.1\r\nHost: " + hostname + "\r\nConnection: close\r\n\r\n";
    if (!sendSSLData(conn, request)) {
        closeSSLConnection(conn);
        cleanupSSL();
        return 1;
    }

    std::string response = receiveSSLData(conn, 8192);
    std::cout << response << std::endl;

    closeSSLConnection(conn);
    cleanupSSL();

    return 0;
}

bool initializeSSL() {
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    return true;
}

SSLConnection* createSSLConnection(const std::string& hostname, int port) {
    SSLConnection* conn = new SSLConnection();
    conn->sockfd = -1;
    conn->ssl = nullptr;
    conn->ctx = nullptr;

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    std::string portStr = std::to_string(port);
    int status = getaddrinfo(hostname.c_str(), portStr.c_str(), &hints, &res);
    if (status != 0) {
        std::cerr << "getaddrinfo error: " << gai_strerror(status) << std::endl;
        delete conn;
        return nullptr;
    }

    conn->sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (conn->sockfd < 0) {
        std::cerr << "Socket creation failed" << std::endl;
        freeaddrinfo(res);
        delete conn;
        return nullptr;
    }

    if (connect(conn->sockfd, res->ai_addr, res->ai_addrlen) < 0) {
        std::cerr << "Connection failed" << std::endl;
        close(conn->sockfd);
        freeaddrinfo(res);
        delete conn;
        return nullptr;
    }
    freeaddrinfo(res);

    const SSL_METHOD* method = TLS_client_method();
    conn->ctx = SSL_CTX_new(method);
    if (!conn->ctx) {
        std::cerr << "SSL_CTX_new failed" << std::endl;
        close(conn->sockfd);
        delete conn;
        return nullptr;
    }

    SSL_CTX_set_verify(conn->ctx, SSL_VERIFY_PEER, nullptr);
    SSL_CTX_set_default_verify_paths(conn->ctx);

    conn->ssl = SSL_new(conn->ctx);
    if (!conn->ssl) {
        std::cerr << "SSL_new failed" << std::endl;
        SSL_CTX_free(conn->ctx);
        close(conn->sockfd);
        delete conn;
        return nullptr;
    }

    SSL_set_fd(conn->ssl, conn->sockfd);
    SSL_set_tlsext_host_name(conn->ssl, hostname.c_str());

    if (SSL_connect(conn->ssl) <= 0) {
        ERR_print_errors_fp(stderr);
        SSL_free(conn->ssl);
        SSL_CTX_free(conn->ctx);
        close(conn->sockfd);
        delete conn;
        return nullptr;
    }

    return conn;
}

bool sendSSLData(SSLConnection* conn, const std::string& data) {
    if (!conn || !conn->ssl) return false;

    int bytesSent = SSL_write(conn->ssl, data.c_str(), data.length());
    if (bytesSent <= 0) {
        int err = SSL_get_error(conn->ssl, bytesSent);
        if (err != SSL_ERROR_WANT_WRITE && err != SSL_ERROR_WANT_READ) {
            return false;
        }
    }
    return true;
}

std::string receiveSSLData(SSLConnection* conn, int maxBytes) {
    if (!conn || !conn->ssl) return "";

    std::string result;
    char buffer[4096];

    while (result.length() < static_cast<size_t>(maxBytes)) {
        int bytesRead = SSL_read(conn->ssl, buffer, sizeof(buffer) - 1);
        if (bytesRead <= 0) {
            int err = SSL_get_error(conn->ssl, bytesRead);
            if (err == SSL_ERROR_ZERO_RETURN || err == SSL_ERROR_SYSCALL) {
                break;
            }
            if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE) {
                break;
            }
        } else {
            buffer[bytesRead] = '\0';
            result.append(buffer, bytesRead);
        }
    }

    return result;
}

void closeSSLConnection(SSLConnection* conn) {
    if (!conn) return;

    if (conn->ssl) {
        SSL_shutdown(conn->ssl);
        SSL_free(conn->ssl);
    }
    if (conn->ctx) {
        SSL_CTX_free(conn->ctx);
    }
    if (conn->sockfd >= 0) {
        close(conn->sockfd);
    }
    delete conn;
}

void cleanupSSL() {
    EVP_cleanup();
    ERR_free_strings();
}