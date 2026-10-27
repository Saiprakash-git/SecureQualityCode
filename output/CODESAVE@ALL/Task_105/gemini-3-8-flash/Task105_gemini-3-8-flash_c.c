#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509v3.h>

int connect_tcp_socket(const char *hostname, const char *port) {
    struct addrinfo hints;
    struct addrinfo *result = NULL;
    struct addrinfo *rp = NULL;
    int sockfd = -1;
    int status;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    status = getaddrinfo(hostname, port, &hints, &result);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        return -1;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sockfd == -1) {
            continue;
        }

        if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1) {
            break;
        }

        close(sockfd);
        sockfd = -1;
    }

    freeaddrinfo(result);
    return sockfd;
}

SSL_CTX *create_ssl_context(void) {
    const SSL_METHOD *method = TLS_client_method();
    SSL_CTX *ctx = SSL_CTX_new(method);

    if (!ctx) {
        fprintf(stderr, "Unable to create SSL context\n");
        ERR_print_errors_fp(stderr);
        return NULL;
    }

    SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);

    if (SSL_CTX_set_default_verify_paths(ctx) != 1) {
        fprintf(stderr, "Failed to load default trust store\n");
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        return NULL;
    }

    return ctx;
}

SSL *establish_ssl_connection(SSL_CTX *ctx, int sockfd, const char *hostname) {
    SSL *ssl = SSL_new(ctx);
    if (!ssl) {
        fprintf(stderr, "Failed to create SSL structure\n");
        ERR_print_errors_fp(stderr);
        return NULL;
    }

    if (SSL_set_fd(ssl, sockfd) != 1) {
        fprintf(stderr, "Failed to associate socket with SSL\n");
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        return NULL;
    }

    if (SSL_set_tlsext_host_name(ssl, hostname) != 1) {
        fprintf(stderr, "Failed to set SNI hostname\n");
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        return NULL;
    }

    if (SSL_set1_host(ssl, hostname) != 1) {
        fprintf(stderr, "Failed to set verification hostname\n");
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        return NULL;
    }

    if (SSL_connect(ssl) <= 0) {
        fprintf(stderr, "SSL handshake failed\n");
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        return NULL;
    }

    return ssl;
}

int send_data(SSL *ssl, const char *data, size_t length) {
    size_t total_sent = 0;
    while (total_sent < length) {
        int bytes_sent = SSL_write(ssl, data + total_sent, (int)(length - total_sent));
        if (bytes_sent <= 0) {
            int err = SSL_get_error(ssl, bytes_sent);
            fprintf(stderr, "SSL_write error code: %d\n", err);
            return -1;
        }
        total_sent += (size_t)bytes_sent;
    }
    return (int)total_sent;
}

int receive_data(SSL *ssl, char *buffer, size_t max_length) {
    int bytes_read = SSL_read(ssl, buffer, (int)(max_length - 1));
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        return bytes_read;
    } else {
        int err = SSL_get_error(ssl, bytes_read);
        if (err != SSL_ERROR_ZERO_RETURN) {
            fprintf(stderr, "SSL_read error code: %d\n", err);
        }
        return bytes_read;
    }
}

void print_connection_info(SSL *ssl) {
    X509 *cert = SSL_get_peer_certificate(ssl);
    if (cert) {
        char *line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("Peer Certificate Subject: %s\n", line);
        free(line);
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("Peer Certificate Issuer: %s\n", line);
        free(line);
        X509_free(cert);
    } else {
        printf("No certificate presented by peer\n");
    }

    printf("TLS Protocol: %s\n", SSL_get_version(ssl));
    printf("Cipher Suite: %s\n", SSL_get_cipher_name(ssl));
}

void teardown(SSL *ssl, SSL_CTX *ctx, int sockfd) {
    if (ssl) {
        SSL_shutdown(ssl);
        SSL_free(ssl);
    }
    if (sockfd != -1) {
        close(sockfd);
    }
    if (ctx) {
        SSL_CTX_free(ctx);
    }
}

int main(int argc, char *argv[]) {
    const char *hostname = "www.example.com";
    const char *port = "443";
    int sockfd = -1;
    SSL_CTX *ctx = NULL;
    SSL *ssl = NULL;
    char request[256];
    char response[4096];
    int req_len;
    int bytes_read;

    if (argc >= 2) {
        hostname = argv[1];
    }
    if (argc >= 3) {
        port = argv[2];
    }

    printf("Connecting to %s:%s via TLS...\n", hostname, port);

    ctx = create_ssl_context();
    if (!ctx) {
        return EXIT_FAILURE;
    }

    sockfd = connect_tcp_socket(hostname, port);
    if (sockfd == -1) {
        teardown(NULL, ctx, -1);
        return EXIT_FAILURE;
    }

    ssl = establish_ssl_connection(ctx, sockfd, hostname);
    if (!ssl) {
        teardown(NULL, ctx, sockfd);
        return EXIT_FAILURE;
    }

    printf("Connection successfully established.\n");
    print_connection_info(ssl);

    req_len = snprintf(request, sizeof(request),
                       "GET / HTTP/1.1\r\n"
                       "Host: %s\r\n"
                       "User-Agent: C-OpenSSL-Client\r\n"
                       "Connection: close\r\n\r\n",
                       hostname);

    if (send_data(ssl, request, (size_t)req_len) < 0) {
        teardown(ssl, ctx, sockfd);
        return EXIT_FAILURE;
    }

    printf("\nSent Request:\n%s\n--- Response Follows ---\n", request);

    while ((bytes_read = receive_data(ssl, response, sizeof(response))) > 0) {
        fwrite(response, 1, (size_t)bytes_read, stdout);
    }

    printf("\n--- End of Response ---\n");

    teardown(ssl, ctx, sockfd);
    return EXIT_SUCCESS;
}