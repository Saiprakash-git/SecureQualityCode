#include <stdio.h>
#include <sqlite3.h>

static int retrieve_user(sqlite3 *db, const char *username, FILE *output)
{
    const char *sql = "SELECT * FROM users WHERE username = ?1";
    sqlite3_stmt *statement = NULL;
    int result;
    int found = 0;

    result = sqlite3_prepare_v2(db, sql, -1, &statement, NULL);
    if (result != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare query: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    result = sqlite3_bind_text(statement, 1, username, -1, SQLITE_TRANSIENT);
    if (result != SQLITE_OK) {
        fprintf(stderr, "Failed to bind username: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(statement);
        return 1;
    }

    while ((result = sqlite3_step(statement)) == SQLITE_ROW) {
        int column;
        found = 1;

        for (column = 0; column < sqlite3_column_count(statement); ++column) {
            const char *name = sqlite3_column_name(statement, column);
            const unsigned char *value = sqlite3_column_text(statement, column);
            int length = sqlite3_column_bytes(statement, column);

            fprintf(output, "%s: ", name ? name : "");
            if (value) {
                fprintf(output, "%.*s", length, (const char *)value);
            } else {
                fprintf(output, "NULL");
            }
            fputc('\n', output);
        }
        fputc('\n', output);
    }

    if (result != SQLITE_DONE) {
        fprintf(stderr, "Failed to execute query: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(statement);
        return 1;
    }

    sqlite3_finalize(statement);

    if (!found) {
        fprintf(output, "No user found for username '%s'.\n", username);
    }

    return 0;
}

int main(int argc, char **argv)
{
    sqlite3 *db = NULL;
    int result;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <database-file> <username>\n", argv[0]);
        return 2;
    }

    result = sqlite3_open_v2(argv[1], &db, SQLITE_OPEN_READONLY, NULL);
    if (result != SQLITE_OK) {
        fprintf(stderr, "Failed to open database: %s\n",
                db ? sqlite3_errmsg(db) : "unable to allocate database handle");
        sqlite3_close(db);
        return 1;
    }

    result = retrieve_user(db, argv[2], stdout);
    sqlite3_close(db);
    return result;
}