#include <stdio.h>
#include <stdlib.h>
#include <libpq-fe.h>

PGconn* create_postgres_connection(const char* host, const char* port,
                                   const char* dbname, const char* user,
                                   const char* password) {
    const char* keys[] = {"host", "port", "dbname", "user", "password", NULL};
    const char* values[] = {host, port, dbname, user, password, NULL};

    PGconn* conn = PQconnectdbParams(keys, values, 0);

    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr, "Connection to database failed: %s\n", PQerrorMessage(conn));
        PQfinish(conn);
        return NULL;
    }

    return conn;
}

int main(int argc, char* argv[]) {
    if (argc != 6) {
        fprintf(stderr, "Usage: %s <host> <port> <dbname> <user> <password>\n", argv[0]);
        return 1;
    }

    PGconn* conn = create_postgres_connection(argv[1], argv[2], argv[3], argv[4], argv[5]);

    if (conn != NULL) {
        printf("Connection established successfully\n");
        PQfinish(conn);
    } else {
        printf("Failed to establish connection\n");
        return 1;
    }

    return 0;
}