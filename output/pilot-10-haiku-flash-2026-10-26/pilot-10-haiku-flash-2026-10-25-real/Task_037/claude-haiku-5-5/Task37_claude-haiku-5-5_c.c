#include <stdio.h>
#include <string.h>
#include <sqlite3.h>

typedef struct {
    int id;
    char username[256];
    char email[256];
    char full_name[256];
} UserInfo;

static void copy_column_text(sqlite3_stmt *stmt, int col, char *dest, size_t size) {
    const unsigned char *text = sqlite3_column_text(stmt, col);
    if (text == NULL) {
        dest[0] = '\0';
    } else {
        snprintf(dest, size, "%s", (const char *)text);
    }
}

int init_schema(sqlite3 *db) {
    const char *sql =
        "CREATE TABLE IF NOT EXISTS users ("
        " id INTEGER PRIMARY KEY,"
        " username TEXT UNIQUE NOT NULL,"
        " email TEXT,"
        " full_name TEXT);";
    char *err = NULL;
    if (sqlite3_exec(db, sql, NULL, NULL, &err) != SQLITE_OK) {
        fprintf(stderr, "Schema error: %s\n", err ? err : "unknown");
        sqlite3_free(err);
        return -1;
    }
    return 0;
}

/* Returns 1 if found, 0 if not found, -1 on error. Uses a parameterized query. */
int fetch_user_info(sqlite3 *db, const char *username, UserInfo *out) {
    const char *sql =
        "SELECT id, username, email, full_name FROM users WHERE username = ?;";
    sqlite3_stmt *stmt = NULL;
    int rc;

    if (db == NULL || username == NULL || out == NULL) {
        return -1;
    }

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Prepare error: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    rc = sqlite3_bind_text(stmt, 1, username, -1, SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Bind error: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return -1;
    }

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        out->id = sqlite3_column_int(stmt, 0);
        copy_column_text(stmt, 1, out->username, sizeof(out->username));
        copy_column_text(stmt, 2, out->email, sizeof(out->email));
        copy_column_text(stmt, 3, out->full_name, sizeof(out->full_name));
        sqlite3_finalize(stmt);
        return 1;
    }

    sqlite3_finalize(stmt);
    if (rc == SQLITE_DONE) {
        return 0;
    }
    fprintf(stderr, "Query error: %s\n", sqlite3_errmsg(db));
    return -1;
}

static void trim_newline(char *s) {
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r')) {
        s[--len] = '\0';
    }
}

int main(int argc, char *argv[]) {
    const char *db_path = (argc > 1) ? argv[1] : "users.db";
    char username[256];
    UserInfo user;
    sqlite3 *db = NULL;
    int rc, result;

    if (sqlite3_open(db_path, &db) != SQLITE_OK) {
        fprintf(stderr, "Cannot open database '%s': %s\n", db_path, sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    if (init_schema(db) != 0) {
        sqlite3_close(db);
        return 1;
    }

    printf("Enter username: ");
    fflush(stdout);
    if (fgets(username, sizeof(username), stdin) == NULL) {
        fprintf(stderr, "No input provided.\n");
        sqlite3_close(db);
        return 1;
    }
    trim_newline(username);

    if (username[0] == '\0') {
        fprintf(stderr, "Username cannot be empty.\n");
        sqlite3_close(db);
        return 1;
    }

    result = fetch_user_info(db, username, &user);
    if (result == 1) {
        printf("ID:        %d\n", user.id);
        printf("Username:  %s\n", user.username);
        printf("Email:     %s\n", user.email);
        printf("Full name: %s\n", user.full_name);
        rc = 0;
    } else if (result == 0) {
        printf("No user found with username '%s'.\n", username);
        rc = 0;
    } else {
        rc = 1;
    }

    sqlite3_close(db);
    return rc;
}