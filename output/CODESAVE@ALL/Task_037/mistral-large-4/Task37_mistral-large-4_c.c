#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql/mysql.h>

#define DB_HOST "localhost"
#define DB_USER "root"
#define DB_PASS "password"
#define DB_NAME "testdb"
#define DB_PORT 3306

int connect_database(MYSQL *conn) {
    if (mysql_init(conn) == NULL) {
        fprintf(stderr, "mysql_init() failed\n");
        return 1;
    }
    if (mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASS, DB_NAME, DB_PORT, NULL, 0) == NULL) {
        fprintf(stderr, "Connection failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        return 1;
    }
    return 0;
}

int query_user(MYSQL *conn, const char *username) {
    MYSQL_STMT *stmt;
    MYSQL_BIND param[1], result[3];
    char query[] = "SELECT username, email, full_name FROM users WHERE username = ?";
    char user[100], email[200], fullname[200];
    unsigned long lengths[3];