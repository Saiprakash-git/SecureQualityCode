#include <libpq-fe.h>
#include <stdio.h>
#include <stdlib.h>

PGconn *create_postgres_connection(const char *host, const char *port, const char *dbname, const char *user, const char *password)
{
    const char *keywords[] = {"host", "port", "dbname", "user", "password", NULL};
    const char *values[5];
    PGconn *conn;

    values[0] = host ? host : "";
    values[1] = port ? port : "";
    values[2] = dbname ? dbname : "";
    values[3] = user ? user : "";
    values[4] = password ? password : "";

    conn = PQconnectdbParams(keywords, values, 0);
    if (conn == NULL) {
        return NULL;
    }

    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr, "Connection to database failed: %s\n", PQerrorMessage(conn));
        PQfinish(conn);
        return NULL;
    }

    return conn;
}

int main(int argc, char *argv[])
{
    const char *host = (argc > 1) ? argv[1] : "localhost";
    const char *port = (argc > 2) ? argv[2] : "5432";
    const char *dbname = (argc > 3) ? argv[3] : "postgres";
    const char *user = (argc > 4) ? argv[4] : getenv("PGUSER");
    const char *password = (argc > 5) ? argv[5] : getenv("PGPASSWORD");

    PGconn *conn = create_postgres_connection(host, port, dbname, user, password);
    if (conn == NULL) {
        return EXIT_FAILURE;
    }

    printf("Connected to database '%s' on host '%s' port '%s'\n",
           PQdb(conn) ? PQdb(conn) : "(unknown)",
           PQhost(conn) ? PQhost(conn) : "(unknown)",
           PQport(conn) ? PQport(conn) : "(unknown)");

    PQfinish(conn);
    return EXIT_SUCCESS;
}