#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_json_string(const unsigned char *text, int length)
{
    int i;

    putchar('"');
    for (i = 0; i < length; ++i) {
        unsigned char c = text[i];

        switch (c) {
            case '"':  fputs("\\\"", stdout); break;
            case '\\': fputs("\\\\", stdout); break;
            case '\b': fputs("\\b", stdout); break;
            case '\f': fputs("\\f", stdout); break;
            case '\n': fputs("\\n", stdout); break;
            case '\r': fputs("\\r", stdout); break;
            case '\t': fputs("\\t", stdout); break;
            default:
                if (c < 0x20) {
                    printf("\\u%04x", (unsigned int)c);
                } else {
                    putchar(c);
                }
        }
    }
    putchar('"');
}

static void print_blob_as_hex(const void *blob, int length)
{
    const unsigned char *bytes = (const unsigned char *)blob;
    int i;

    putchar('"');
    for (i = 0; i < length; ++i) {
        printf("%02x", bytes[i]);
    }
    putchar('"');
}

static void print_column_value(sqlite3_stmt *statement, int column)
{
    int type = sqlite3_column_type(statement, column);

    switch (type) {
        case SQLITE_INTEGER:
            printf("%lld", (long long)sqlite3_column_int64(statement, column));
            break;
        case SQLITE_FLOAT:
            printf("%.17g", sqlite3_column_double(statement, column));
            break;
        case SQLITE_TEXT: {
            const unsigned char *text = sqlite3_column_text(statement, column);
            int length = sqlite3_column_bytes(statement, column);
            print_json_string(text ? text : (const unsigned char *)"", length);
            break;
        }
        case SQLITE_BLOB: {
            const void *blob = sqlite3_column_blob(statement, column);
            int length = sqlite3_column_bytes(statement, column);
            print_blob_as_hex(blob, length);
            break;
        }
        case SQLITE_NULL:
        default:
            fputs("null", stdout);
            break;
    }
}

int main(int argc, char **argv)
{
    sqlite3 *database = NULL;
    sqlite3_stmt *statement = NULL;
    const char *database_path = "customer.db";
    const char *sql =
        "SELECT * FROM customer WHERE customerusername = ?1 LIMIT 1";
    int result;
    int column_count;
    int column;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <customerusername>\n", argv[0]);
        return EXIT_FAILURE;
    }

    result = sqlite3_open(database_path, &database);
    if (result != SQLITE_OK) {
        fprintf(stderr, "Database error: %s\n",
                database ? sqlite3_errmsg(database) : "unable to open database");
        sqlite3_close(database);
        return EXIT_FAILURE;
    }

    result = sqlite3_prepare_v2(database, sql, -1, &statement, NULL);
    if (result != SQLITE_OK) {
        fprintf(stderr, "Database error: %s\n", sqlite3_errmsg(database));
        sqlite3_close(database);
        return EXIT_FAILURE;
    }

    result = sqlite3_bind_text(statement, 1, argv[1], -1, SQLITE_TRANSIENT);
    if (result != SQLITE_OK) {
        fprintf(stderr, "Database error: %s\n", sqlite3_errmsg(database));
        sqlite3_finalize(statement);
        sqlite3_close(database);
        return EXIT_FAILURE;
    }

    result = sqlite3_step(statement);
    if (result == SQLITE_ROW) {
        column_count = sqlite3_column_count(statement);
        putchar('{');

        for (column = 0; column < column_count; ++column) {
            const char *name = sqlite3_column_name(statement, column);

            if (column > 0) {
                putchar(',');
            }
            print_json_string((const unsigned char *)(name ? name : ""),
                              (int)strlen(name ? name : ""));
            putchar(':');
            print_column_value(statement, column);
        }

        puts("}");
    } else if (result == SQLITE_DONE) {
        puts("null");
    } else {
        fprintf(stderr, "Database error: %s\n", sqlite3_errmsg(database));
        sqlite3_finalize(statement);
        sqlite3_close(database);
        return EXIT_FAILURE;
    }

    sqlite3_finalize(statement);
    sqlite3_close(database);
    return EXIT_SUCCESS;
}