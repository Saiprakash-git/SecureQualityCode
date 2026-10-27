#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

#define RESPONSE_CHUNK 4096

static int tcp_connect(const char *host, const char *port)
{
    struct addrinfo hints, *res = NULL, *p = NULL;
    int fd = -1;
    int rc;

    if (!host || !port)
        return -1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    rc = getaddrinfo(host, port, &hints, &res);
    if (rc != 0) {
        fprintf(stderr, "getaddrinfo(%s): %s\n", host, gai_strerror(rc));
        return -1;
    }

    for (p = res; p != NULL; p = p->ai_next) {
        fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (fd < 0)
            continue;
        if (connect(fd, p->ai_addr, p->ai_addrlen) == 0)
            break;
        close(fd);
        fd = -1;
    }

    freeaddrinfo(res);
    return fd;
}

static SSL_CTX *create_tls_context(const char *ca_file)
{
    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx) {
        ERR_print_errors_fp(stderr);
        return NULL;
    }

    if (SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION) != 1) {
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        return NULL;
    }

    if (SSL_CTX_set_cipher_list(ctx,
            "ECDHE+AESGCM:ECDHE+CHACHA20:!aNULL:!eNULL:!MD5:!RC4:!3DES:!DES") != 1) {
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        return NULL;
    }

    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);

    if (ca_file) {
        if (SSL_CTX_load_verify_locations(ctx, ca_file, NULL) != 1) {
            fprintf(stderr, "Failed to load CA file: %s\n", ca_file);
            ERR_print_errors_fp(stderr);
            SSL_CTX_free(ctx);
            return NULL;
        }
    } else {
        if (SSL_CTX_set_default_verify_paths(ctx) != 1) {
            fprintf(stderr, "Failed to load default CA paths\n");
            ERR_print_errors_fp(stderr);
            SSL_CTX_free(ctx);
            return NULL;
        }
    }

    return ctx;
}

static SSL *tls_connect(SSL_CTX *ctx, int fd, const char *host)
{
    SSL *ssl = NULL;
    X509_VERIFY_PARAM *param = NULL;
    unsigned char addr_buf[16];
    long verify_result;

    ssl = SSL_new(ctx);
    if (!ssl) {
        ERR_print_errors_fp(stderr);
        return NULL;
    }

    if (SSL_set_fd(ssl, fd) != 1) {
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        return NULL;
    }

    /* Server Name Indication, only for hostnames (not IP literals) */
    if (inet_pton(AF_INET, host, addr_buf) != 1 &&
        inet_pton(AF_INET6, host, addr_buf) != 1) {
        SSL_set_tlsext_host_name(ssl, host);
    }

    /* Hostname / IP verification against the certificate */
    param = SSL_get0_param(ssl);
    if (inet_pton(AF_INET, host, addr_buf) == 1 ||
        inet_pton(AF_INET6, host, addr_buf) == 1) {
        X509_VERIFY_PARAM_set1_ip_asc(param, host);
    } else {
        X509_VERIFY_PARAM_set_hostflags(param, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
        X509_VERIFY_PARAM_set1_host(param, host, 0);
    }

    if (SSL_connect(ssl) != 1) {
        fprintf(stderr, "TLS handshake failed with %s\n", host);
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        return NULL;
    }

    verify_result = SSL_get_verify_result(ssl);
    if (verify_result != X509_V_OK) {
        fprintf(stderr, "Certificate verification failed: %s\n",
                X509_verify_cert_error_string(verify_result));
        SSL_shutdown(ssl);
        SSL_free(ssl);
        return NULL;
    }

    return ssl;
}

static void print_connection_info(SSL *ssl)
{
    X509 *cert = SSL_get1_peer_certificate(ssl);
    char *subject = NULL;
    char *issuer = NULL;

    printf("Protocol : %s\n", SSL_get_version(ssl));
    printf("Cipher   : %s\n", SSL_get_cipher(ssl));

    if (cert) {
        subject = X509_NAME_oneline(X509_get_subject_name(cert), NULL, 0);
        issuer = X509_NAME_oneline(X509_get_issuer_name(cert), NULL, 0);
        printf("Subject  : %s\n", subject ? subject : "(unknown)");
        printf("Issuer   : %s\n", issuer ? issuer : "(unknown)");
        OPENSSL_free(subject);
        OPENSSL_free(issuer);
        X509_free(cert);
    }
}

static int https_get(SSL *ssl, const char *host, const char *path,
                     char **response, size_t *response_len)
{
    char request[1024];
    size_t total = 0, cap = RESPONSE_CHUNK;
    char *buf = NULL, *tmp = NULL;
    int n, written;

    if (!ssl || !host || !path || !response || !response_len)
        return -1;

    *response = NULL;
    *response_len = 0;

    written = snprintf(request, sizeof(request),
                       "GET %s HTTP/1.1\r\n"
                       "Host: %s\r\n"
                       "User-Agent: secure-tls-client/1.0\r\n"
                       "Connection: close\r\n\r\n",
                       path, host);
    if (written < 0 || (size_t)written >= sizeof(request))
        return -1;