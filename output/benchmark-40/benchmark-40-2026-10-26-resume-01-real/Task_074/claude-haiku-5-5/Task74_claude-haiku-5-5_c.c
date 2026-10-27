#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libpq-fe.h>

typedef struct {
    PGconn *conn;
} pg_connection_t;

static const char *nonempty_or_null(const char *s)
{
    return (s != NULL && s[0] != '\0') ? s : NULL;
}

static void set_error(char *errbuf, size_t errlen, const char *msg)
{
    if (errbuf == NULL || errlen == 0) {
        return;
    }
    strncpy(errbuf, msg != NULL ? msg : "unknown error", errlen - 1);
    errbuf[errlen - 1] = '\0';
}

pg_connection_t *pg_connection_create(const char *host,
                                      const char *port,
                                      const char *dbname,
                                      const char *user,
                                      const char *password,
                                      char *errbuf,
                                      size_t errlen)
{
    const char *keywords[] = { "host", "port", "dbname", "user", "password", NULL };
    const char *values[]   = {
        nonempty_or_null(host),
        nonempty_or_null(port),
        nonempty_or_null(dbname),
        nonempty_or_null(user),
        nonempty_or_null(password),
        NULL
    };

    pg_connection_t *obj = malloc(sizeof(*obj));
    if (obj == NULL) {
        set_error(errbuf, errlen, "memory allocation failed");
        return NULL;
    }

    obj->conn = PQconnectdbParams(keywords, values, 0);
    if (obj->conn == NULL) {
        set_error(errbuf, errlen, "failed to allocate PGconn");
        free(obj);
        return NULL;
    }

    if (PQstatus(obj->conn) != CONNECTION_OK) {
        set_error(errbuf, errlen, PQerrorMessage(obj->conn));
        PQfinish(obj->conn);
        free(obj);
        return NULL;
    }

    return obj;
}

int pg_connection_is_alive(const pg_connection_t *obj)
{
    if (obj == NULL || obj->conn == NULL) {
        return 0;
    }
    return PQstatus(obj->conn) == CONNECTION_OK;
}

int pg_connection_server_version(const pg_connection_t *obj)
{
    if (!pg_connection_is_alive(obj)) {
        return -1;
    }
    return PQserverVersion(obj->conn);
}

void pg_connection_destroy(pg_connection_t *obj)
{
    if (obj == NULL) {
        return;
    }
    if (obj->conn != NULL) {
        PQfinish(obj->conn);
    }
    free(obj);
}

int main(int argc, char *argv[])
{
    const char *host     = argc > 1 ? argv[1] : NULL;
    const char *port     = argc > 2 ? argv[2] : NULL;
    const char *dbname   = argc > 3 ? argv[3] : NULL;
    const char *user     = argc > 4 ? argv[4] : NULL;
    const char *password = argc > 5 ? argv[5] : NULL;

    char errbuf[512];
    pg_connection_t *db = pg_connection_create(host, port, dbname, user, password,
                                               errbuf, sizeof(errbuf));
    if (db == NULL) {
        fprintf(stderr, "Connection failed: %s\n", errbuf);
        return 1;
    }

    printf("Connected successfully. Server version: %d\n",
           pg_connection_server_version(db));

    pg_connection_destroy(db);
    return 0;
}