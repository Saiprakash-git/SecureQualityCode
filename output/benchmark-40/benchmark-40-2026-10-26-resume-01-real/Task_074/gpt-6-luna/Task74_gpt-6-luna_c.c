#include <libpq-fe.h>

PGconn *create_postgres_connection(const char *connection_info)
{
    if (connection_info == NULL) {
        return NULL;
    }

    PGconn *connection = PQconnectdb(connection_info);
    if (connection == NULL) {
        return NULL;
    }

    if (PQstatus(connection) != CONNECTION_OK) {
        PQfinish(connection);
        return NULL;
    }

    return connection;
}