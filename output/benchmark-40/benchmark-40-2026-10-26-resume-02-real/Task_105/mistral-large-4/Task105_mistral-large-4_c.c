#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/opensslv.h>

typedef struct {
    SSL_CTX *ctx;
    SSL *ssl;
    int sockfd;
} ssl_connection_t;

int ssl_init(void);
ssl_connection_t* ssl_connect(const char *hostname, int port);
int ssl_send(ssl_connection_t *conn, const char *data, int length);
int ssl_receive(ssl_connection_t *conn, char *buffer, int buffer_size);
void ssl_disconnect(ssl_connection_t *conn);
void ssl_cleanup(void);
void print_ssl_errors(void);

void print_ssl_errors(void) {
    ERR_print_errors_fp(stderr);
}

int ssl_init(void) {
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
#endif
    return 0;
}

ssl_connection_t* ssl_connect(const char *hostname, int port) {
    struct hostent *host;
    struct sockaddr_in addr;
    ssl_connection_t *conn;

    conn = (ssl_connection_t*)malloc(sizeof(ssl_connection_t));