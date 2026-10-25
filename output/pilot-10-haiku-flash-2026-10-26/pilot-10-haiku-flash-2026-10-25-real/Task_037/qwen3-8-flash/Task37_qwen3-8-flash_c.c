#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

static void trim_newline(char *s)
{
    size_t len;

    if (s == NULL)
        return;

    len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r')) {
        s[len - 1] = '\0';
        len--;
    }
}

static int retrieve_user_info(const char *db_path, const char *username)
{
    sqlite3 *db;
    sqlite3_stmt *stmt;
    char *errmsg;
    int rc;
    const char *sql =
        "SELECT id, username, email, created_at "
        "FROM users "
        "WHERE username = ?;";

    if (db_path == NULL || username == NULL || username[0] == '\0') {
        fprintf(stderr, "Invalid database path or username.\n");
        return 1;
    }

    rc = sqlite3_open(db_path, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }

    rc = sqlite3_bind_text(stmt, 1, username, -1, SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to bind username: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        const unsigned char *id = sqlite3_column_text(stmt, 0);
        const unsigned char *db_username = sqlite3_column_text(stmt, 1);
        const unsigned char *email = sqlite3_column_text(stmt, 2);
        const unsigned char *created_at = sqlite3_column_text(stmt, 3);

        printf("User found:\n");
        printf("  ID:         %s\n", id ? (const char *)id : "(null)");
        printf("  Username:   %s\n", db_username ? (const char *)db_username : "(null)");
        printf("  Email:      %s\n", email ? (const char *)email : "(null)");
        printf("  Created At: %s\n", created_at ? (const char *)created_at : "(null)");
    } else if (rc == SQLITE_DONE) {
        printf("No user found for username: %s\n", username);
    } else {
        errmsg = sqlite3_errmsg(db);
        fprintf(stderr, "Query failed: %s\n", errmsg);
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return 0;
}

int main(int argc, char **argv)
{
    const char *db_path;
    char username[256];

    db_path = (argc > 1) ? argv[1] : "users.db";

    printf("Enter username: ");
    fflush(stdout);

    if (fgets(username, sizeof(username), stdin) == NULL) {
        fprintf(stderr, "Failed to read username.\n");
        return 1;
    }

    trim_newline(username);

    if (username[0] == '\0') {
        fprintf(stderr, "Username cannot be empty.\n");
        return 1;
    }

    return retrieve_user_info(db_path, username);
}