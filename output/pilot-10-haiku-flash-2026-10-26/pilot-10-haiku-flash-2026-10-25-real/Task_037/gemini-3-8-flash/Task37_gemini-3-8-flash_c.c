#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

typedef struct {
    int id;
    char username[64];
    char email[128];
    char created_at[32];
} UserInfo;

int setup_database(sqlite3 **db) {
    int rc = sqlite3_open(":memory:", db);
    if (rc != SQLITE_OK) {
        return rc;
    }

    const char *create_table_sql = 
        "CREATE TABLE users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "username TEXT UNIQUE NOT NULL, "
        "email TEXT NOT NULL, "
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
        ");";

    char *err_msg = NULL;
    rc = sqlite3_exec(*db, create_table_sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        sqlite3_free(err_msg);
        return rc;
    }

    const char *seed_sql = 
        "INSERT INTO users (username, email) VALUES ('alice', 'alice@example.com');"
        "INSERT INTO users (username, email) VALUES ('bob', 'bob@example.com');"
        "INSERT INTO users (username, email) VALUES ('charlie', 'charlie@example.com');";

    rc = sqlite3_exec(*db, seed_sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        sqlite3_free(err_msg);
        return rc;
    }

    return SQLITE_OK;
}

int query_user_by_username(sqlite3 *db, const char *username, UserInfo *user) {
    const char *sql = "SELECT id, username, email, created_at FROM users WHERE username = ?;";
    sqlite3_stmt *stmt = NULL;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return -1;
    }

    rc = sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        sqlite3_finalize(stmt);
        return -1;
    }

    int found = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        user->id = sqlite3_column_int(stmt, 0);

        const unsigned char *uname = sqlite3_column_text(stmt, 1);
        if (uname) {
            strncpy(user->username, (const char *)uname, sizeof(user->username) - 1);
            user->username[sizeof(user->username) - 1] = '\0';
        }

        const unsigned char *email = sqlite3_column_text(stmt, 2);
        if (email) {
            strncpy(user->email, (const char *)email, sizeof(user->email) - 1);
            user->email[sizeof(user->email) - 1] = '\0';
        }

        const unsigned char *created_at = sqlite3_column_text(stmt, 3);
        if (created_at) {
            strncpy(user->created_at, (const char *)created_at, sizeof(user->created_at) - 1);
            user->created_at[sizeof(user->created_at) - 1] = '\0';
        }

        found = 1;
    }

    sqlite3_finalize(stmt);
    return found;
}

int main(void) {
    sqlite3 *db = NULL;
    char target_username[64];
    UserInfo user;

    if (setup_database(&db) != SQLITE_OK) {
        fprintf(stderr, "Failed to initialize database: %s\n", sqlite3_errmsg(db));
        if (db) sqlite3_close(db);
        return EXIT_FAILURE;
    }

    printf("Enter username to search (e.g., alice, bob, charlie): ");
    if (fgets(target_username, sizeof(target_username), stdin) == NULL) {
        sqlite3_close(db);
        return EXIT_FAILURE;
    }

    target_username[strcspn(target_username, "\r\n")] = '\0';

    if (strlen(target_username) == 0) {
        printf("Username cannot be empty.\n");
        sqlite3_close(db);
        return EXIT_SUCCESS;
    }

    memset(&user, 0, sizeof(UserInfo));
    int result = query_user_by_username(db, target_username, &user);

    if (result == 1) {
        printf("\n--- User Record Found ---\n");
        printf("ID:         %d\n", user.id);
        printf("Username:   %s\n", user.username);
        printf("Email:      %s\n", user.email);
        printf("Created At: %s\n", user.created_at);
    } else if (result == 0) {
        printf("\nNo record found for username '%s'.\n", target_username);
    } else {
        fprintf(stderr, "\nAn error occurred while executing the query.\n");
    }

    sqlite3_close(db);
    return EXIT_SUCCESS;
}