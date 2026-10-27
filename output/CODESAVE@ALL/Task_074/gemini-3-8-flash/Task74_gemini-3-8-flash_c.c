#include <stdio.h>
#include <stdlib.h>
#include <libpq-fe.h>

PGconn *create_postgres_connection(const char *host,
                                   const char *port,
                                   const char *dbname,
                                   const char *user,
                                   const char *password)
{
    const char *keywords[] = {
        "host",
        "port",
        "dbname",
        "user",
        "password",
        NULL
    };

    const char *values[] = {
        host,
        port,
        dbname,
        user,
        password,
        NULL
    };

    PGconn *conn = PQconnectdbParams(keywords, values, 0);

    if (conn == NULL) {
        fprintf(stderr, "Memory allocation error while creating connection structure.\n");
        return NULL;
    }

    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr, "Connection to database failed: %s", PQerrorMessage(conn));
        PQfinish(conn);
        return NULL;
    }

    return conn;
}

void close_postgres_connection(PGconn *conn)
{
    if (conn != NULL) {
        PQfinish(conn);
    }
}

int main(int argc, char *argv[])
{
    const char *host = (argc > 1) ? argv[1] : "localhost";
    const char *port = (argc > 2) ? argv[2] : "5432";
    const char *dbname = (argc > 3) ? argv[3] : "postgres";
    const char *user = (argc > 4) ? argv[4] : "postgres";
    const char *password = (argc > 5) ? argv[5] : "";

    printf("Connecting to PostgreSQL at %s:%s, dbname=%s, user=%s...\n",
           host, port, dbname, user);

    PGconn *conn = create_postgres_connection(host, port, dbname, user, password);

    if (conn == NULL) {
        fprintf(stderr, "Failed to establish PostgreSQL connection.\n");
        return EXIT_FAILURE;
    }

    printf("Connection successfully established!\n");
    printf("Server version: %d\n", PQserverVersion(conn));
    printf("Protocol version: %d\n", PQprotocolVersion(conn));

    close_postgres_connection(conn);
    printf("Connection closed.\n");

    return EXIT_SUCCESS;
}