#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/x509_vfy.h>

static int connect_tcp(const char *hostname, const char *port)
{
    struct addrinfo hints;
    struct addrinfo *results = NULL;
    struct addrinfo *entry;
    int fd = -1;
    int result;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    result = getaddrinfo(hostname, port, &hints, &results);
    if (result != 0) {
        fprintf(stderr, "Address lookup failed: %s\n", gai_strerror(result));
        return -1;
    }

    for (entry = results; entry != NULL; entry = entry->ai_next) {
        fd = socket(entry->ai_family, entry->ai_socktype, entry->ai_protocol);
        if (fd < 0)
            continue;

        if (connect(fd, entry->ai_addr, entry->ai_addrlen) == 0)
            break;

        close(fd);
        fd = -1;
    }

    freeaddrinfo(results);

    if (fd < 0)
        fprintf(stderr, "Could not connect to %s:%s: %s\n",
                hostname, port, strerror(errno));

    return fd;
}

static int configure_peer_identity(SSL *ssl, const char *hostname)
{
    unsigned char address[sizeof(struct in6_addr)];
    X509_VERIFY_PARAM *parameters = SSL_get0_param(ssl);

    if (inet_pton(AF_INET, hostname, address) == 1 ||
        inet_pton(AF_INET6, hostname, address) == 1) {
        return X509_VERIFY_PARAM_set1_ip_asc(parameters, hostname) == 1;
    }

    if (SSL_set_tlsext_host_name(ssl, hostname) != 1)
        return 0;

    return SSL_set1_host(ssl, hostname) == 1;
}

static void report_tls_errors(const char *operation)
{
    fprintf(stderr, "%s failed\n", operation);
    ERR_print_errors_fp(stderr);
}

int main(int argc, char **argv)
{
    const char *hostname;
    const char *port;
    SSL_CTX *context = NULL;
    SSL *ssl = NULL;
    int socket_fd = -1;
    int exit_status = EXIT_FAILURE;
    struct sigaction ignore_sigpipe;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s hostname port\n", argv[0]);
        return EXIT_FAILURE;
    }

    hostname = argv[1];
    port = argv[2];

    memset(&ignore_sigpipe, 0, sizeof(ignore_sigpipe));
    ignore_sigpipe.sa_handler = SIG_IGN;
    sigemptyset(&ignore_sigpipe.sa_mask);
    sigaction(SIGPIPE, &ignore_sigpipe, NULL);

    if (OPENSSL_init_ssl(0, NULL) != 1) {
        report_tls_errors("OpenSSL initialization");
        goto cleanup;
    }

    context = SSL_CTX_new(TLS_client_method());
    if (context == NULL) {
        report_tls_errors("TLS context creation");
        goto cleanup;
    }

    if (SSL_CTX_set_min_proto_version(context, TLS1_2_VERSION) != 1) {
        report_tls_errors("Setting minimum TLS version");
        goto cleanup;
    }

    SSL_CTX_set_verify(context, SSL_VERIFY_PEER, NULL);
    if (SSL_CTX_set_default_verify_paths(context) != 1) {
        report_tls_errors("Loading trusted certificate authorities");
        goto cleanup;
    }

    SSL_CTX_set_options(context, SSL_OP_NO_COMPRESSION);
#ifdef SSL_OP_NO_RENEGOTIATION
    SSL_CTX_set_options(context, SSL_OP_NO_RENEGOTIATION);
#endif

    socket_fd = connect_tcp(hostname, port);
    if (socket_fd < 0)
        goto cleanup;

    ssl = SSL_new(context);
    if (ssl == NULL) {
        report_tls_errors("TLS session creation");
        goto cleanup;
    }

    if (configure_peer_identity(ssl, hostname) != 1) {
        report_tls_errors("Configuring server identity verification");
        goto cleanup;
    }

    if (SSL_set_fd(ssl, socket_fd) != 1) {
        report_tls_errors("Associating TLS session with socket");
        goto cleanup;
    }

    if (SSL_connect(ssl) != 1) {
        report_tls_errors("TLS handshake");
        goto cleanup;
    }

    if (SSL_get_verify_result(ssl) != X509_V_OK) {
        fprintf(stderr, "Server certificate verification failed: %s\n",
                X509_verify_cert_error_string(SSL_get_verify_result(ssl)));
        goto cleanup;
    }

    printf("Secure connection established to %s:%s using %s (%s)\n",
           hostname, port, SSL_get_version(ssl), SSL_get_cipher(ssl));
    exit_status = EXIT_SUCCESS;

cleanup:
    if (ssl != NULL)
        SSL_free(ssl);
    if (socket_fd >= 0)
        close(socket_fd);
    if (context != NULL)
        SSL_CTX_free(context);

    return exit_status;
}