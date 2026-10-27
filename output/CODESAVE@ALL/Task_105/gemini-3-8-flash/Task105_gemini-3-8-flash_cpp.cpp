#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <memory>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509v3.h>

struct TlsConnection {
    SSL_CTX* ctx = nullptr;
    SSL* ssl = nullptr;
    int socket_fd = -1;
    bool is_connected = false;
};

void print_ssl_errors() {
    unsigned long err_code;
    while ((err_code = ERR_get_error()) != 0) {
        char err_buf[256];
        ERR_error_string_n(err_code, err_buf, sizeof(err_buf));
        std::cerr << "OpenSSL Error: " << err_buf << std::endl;
    }
}

SSL_CTX* create_ssl_context() {
    const SSL_METHOD* method = TLS_client_method();
    SSL_CTX* ctx = SSL_CTX_new(method);
    if (!ctx) {
        std::cerr << "Failed to create SSL context." << std::endl;
        print_ssl_errors();
        return nullptr;
    }

    // Require TLS 1.2 or TLS 1.3
    if (SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION) != 1) {
        std::cerr << "Failed to set minimum TLS version." << std::endl;
        SSL_CTX_free(ctx);
        return nullptr;
    }

    // Load default trusted CA certificates
    if (SSL_CTX_set_default_verify_paths(ctx) != 1) {
        std::cerr << "Failed to load system default CA verification paths." << std::endl;
    }

    // Enforce certificate validation
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, nullptr);

    return ctx;
}

int create_tcp_socket(const std::string& hostname, int port) {
    struct addrinfo hints;
    struct addrinfo* result = nullptr;
    struct addrinfo* rp = nullptr;

    std::memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    std::string port_str = std::to_string(port);
    int status = getaddrinfo(hostname.c_str(), port_str.c_str(), &hints, &result);
    if (status != 0) {
        std::cerr << "getaddrinfo error: " << gai_strerror(status) << std::endl;
        return -1;
    }

    int socket_fd = -1;
    for (rp = result; rp != nullptr; rp = rp->ai_next) {
        socket_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (socket_fd == -1) {
            continue;
        }

        if (connect(socket_fd, rp->ai_addr, rp->ai_addrlen) != -1) {
            break;
        }

        close(socket_fd);
        socket_fd = -1;
    }

    freeaddrinfo(result);
    return socket_fd;
}

TlsConnection connect_tls(const std::string& hostname, int port) {
    TlsConnection conn;

    conn.ctx = create_ssl_context();
    if (!conn.ctx) {
        return conn;
    }

    conn.socket_fd = create_tcp_socket(hostname, port);
    if (conn.socket_fd < 0) {
        std::cerr << "Failed to establish TCP connection to " << hostname << ":" << port << std::endl;
        SSL_CTX_free(conn.ctx);
        conn.ctx = nullptr;
        return conn;
    }

    conn.ssl = SSL_new(conn.ctx);
    if (!conn.ssl) {
        std::cerr << "Failed to create SSL structure." << std::endl;
        print_ssl_errors();
        close(conn.socket_fd);
        SSL_CTX_free(conn.ctx);
        conn.ctx = nullptr;
        conn.socket_fd = -1;
        return conn;
    }

    // Set Server Name Indication (SNI)
    if (SSL_set_tlsext_host_name(conn.ssl, hostname.c_str()) != 1) {
        std::cerr << "Failed to set SNI hostname." << std::endl;
        print_ssl_errors();
    }

    // Set hostname verification for certificate validation
    if (SSL_set1_host(conn.ssl, hostname.c_str()) != 1) {
        std::cerr << "Failed to set verification hostname." << std::endl;
        print_ssl_errors();
    }

    if (SSL_set_fd(conn.ssl, conn.socket_fd) != 1) {
        std::cerr << "Failed to attach socket to SSL structure." << std::endl;
        print_ssl_errors();
        SSL_free(conn.ssl);
        close(conn.socket_fd);
        SSL_CTX_free(conn.ctx);
        conn.ssl = nullptr;
        conn.ctx = nullptr;
        conn.socket_fd = -1;
        return conn;
    }

    // Perform TLS Handshake
    int handshake_res = SSL_connect(conn.ssl);
    if (handshake_res <= 0) {
        int err = SSL_get_error(conn.ssl, handshake_res);
        std::cerr << "TLS handshake failed with error code: " << err << std::endl;
        print_ssl_errors();
        SSL_free(conn.ssl);
        close(conn.socket_fd);
        SSL_CTX_free(conn.ctx);
        conn.ssl = nullptr;
        conn.ctx = nullptr;
        conn.socket_fd = -1;
        return conn;
    }

    // Verify certificate result
    long verify_result = SSL_get_verify_result(conn.ssl);
    if (verify_result != X509_V_OK) {
        std::cerr << "Certificate verification failed: " 
                  << X509_verify_cert_error_string(verify_result) << std::endl;
        SSL_shutdown(conn.ssl);
        SSL_free(conn.ssl);
        close(conn.socket_fd);
        SSL_CTX_free(conn.ctx);
        conn.ssl = nullptr;
        conn.ctx = nullptr;
        conn.socket_fd = -1;
        return conn;
    }

    conn.is_connected = true;
    return conn;
}

bool send_tls_data(const TlsConnection& conn, const std::string& data) {
    if (!conn.is_connected || !conn.ssl) {
        return false;
    }

    size_t total_sent = 0;
    while (total_sent < data.size()) {
        int sent = SSL_write(conn.ssl, data.data() + total_sent, data.size() - total_sent);
        if (sent <= 0) {
            int err = SSL_get_error(conn.ssl, sent);
            std::cerr << "SSL_write failed with code: " << err << std::endl;
            print_ssl_errors();
            return false;
        }
        total_sent += sent;
    }

    return true;
}

std::string receive_tls_data(const TlsConnection& conn, size_t max_bytes) {
    if (!conn.is_connected || !conn.ssl) {
        return "";
    }

    std::string received;
    std::vector<char> buffer(4096);

    while (received.size() < max_bytes) {
        int bytes_read = SSL_read(conn.ssl, buffer.data(), buffer.size());
        if (bytes_read > 0) {
            received.append(buffer.data(), bytes_read);
        } else {
            int err = SSL_get_error(conn.ssl, bytes_read);
            if (err == SSL_ERROR_ZERO_RETURN) {
                // Connection closed cleanly by remote peer
                break;
            } else if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
                continue;
            } else {
                break;
            }
        }
    }

    return received;
}

void close_tls_connection(TlsConnection& conn) {
    if (conn.ssl) {
        SSL_shutdown(conn.ssl);
        SSL_free(conn.ssl);
        conn.ssl = nullptr;
    }
    if (conn.socket_fd >= 0) {
        close(conn.socket_fd);
        conn.socket_fd = -1;
    }
    if (conn.ctx) {
        SSL_CTX_free(conn.ctx);
        conn.ctx = nullptr;
    }
    conn.is_connected = false;
}

int main(int argc, char* argv[]) {
    std::string hostname = "example.com";
    int port = 443;
    std::string path = "/";

    if (argc >= 2) {
        hostname = argv[1];
    }
    if (argc >= 3) {
        port = std::stoi(argv[2]);
    }
    if (argc >= 4) {
        path = argv[3];
    }

    std::cout << "Connecting to " << hostname << ":" << port << " over TLS..." << std::endl;
    TlsConnection conn = connect_tls(hostname, port);

    if (!conn.is_connected) {
        std::cerr << "Failed to establish TLS connection." << std::endl;
        return 1;
    }

    std::cout << "Connected securely using cipher: " << SSL_get_cipher(conn.ssl) << std::endl;
    std::cout << "TLS Version: " << SSL_get_version(conn.ssl) << std::endl;

    X509* cert = SSL_get_peer_certificate(conn.ssl);
    if (cert) {
        char* subj = X509_NAME_oneline(X509_get_subject_name(cert), nullptr, 0);
        char* issuer = X509_NAME_oneline(X509_get_issuer_name(cert), nullptr, 0);
        if (subj) {
            std::cout << "