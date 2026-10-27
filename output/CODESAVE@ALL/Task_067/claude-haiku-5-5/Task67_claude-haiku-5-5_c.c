#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

#define DEFAULT_DB_PATH "customers.db"
#define FIELD_LEN 256

typedef struct {
    int customer_id;
    char customerusername[FIELD_LEN];
    char first_name[FIELD_LEN];
    char last_name[FIELD_LEN];
    char email[FIELD_LEN];
    char phone[FIELD_LEN];
    char address[FIELD_LEN];
} Customer;

static void copy_column_text(sqlite3_stmt *stmt, int col, char *dest, size_t dest_size) {
    const unsigned char *text = sqlite3_column_text(stmt, col);
    if (text) {
        strncpy(dest, (const char *)text, dest_size - 1);
        dest[dest_size - 1] = '\0';
    } else {
        dest[0] = '\0';
    }
}

/*
 * Retrieves a customer record by customerusername.
 * Returns 0 on success (customer found), 1 if not found, -1 on error.
 */
int fetch_customer(const char *db_path, const char *username, Customer *out) {
    sqlite3 *db = NULL;
    sqlite3_stmt *stmt = NULL;
    const char *sql =
        "SELECT customer_id, customerusername, first_name, last_name, "
        "email, phone, address "
        "FROM customer WHERE customerusername = ? LIMIT 1;";
    int rc;
    int result = -1;

    if (!db_path || !username || !out) {
        return -1;
    }

    rc = sqlite3_open_v2(db_path, &db, SQLITE_OPEN_READONLY, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error opening database '%s': %s\n",
                db_path, sqlite3_errmsg(db));
        sqlite3_close(db);
        return -1;
    }

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error preparing statement: %s\n", sqlite3_errmsg(db));
        goto cleanup;
    }

    rc = sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error binding parameter: %s\n", sqlite3_errmsg(db));
        goto cleanup;
    }

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        memset(out, 0, sizeof(*out));
        out->customer_id = sqlite3_column_int(stmt, 0);
        copy_column_text(stmt, 1, out->customerusername, FIELD_LEN);
        copy_column_text(stmt, 2, out->first_name, FIELD_LEN);
        copy_column_text(stmt, 3, out->last_name, FIELD_LEN);
        copy_column_text(stmt, 4, out->email, FIELD_LEN);
        copy_column_text(stmt, 5, out->phone, FIELD_LEN);
        copy_column_text(stmt, 6, out->address, FIELD_LEN);
        result = 0;
    } else if (rc == SQLITE_DONE) {
        result = 1;
    } else {
        fprintf(stderr, "Error executing query: %s\n", sqlite3_errmsg(db));
        result = -1;
    }

cleanup:
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return result;
}

void print_customer(const Customer *c) {
    printf("Customer ID : %d\n", c->customer_id);
    printf("Username    : %s\n", c->customerusername);
    printf("First Name  : %s\n", c->first_name);
    printf("Last Name   : %s\n", c->last_name);
    printf("Email       : %s\n", c->email);
    printf("Phone       : %s\n", c->phone);
    printf("Address     : %s\n", c->address);
}

int main(int argc, char *argv[]) {
    Customer customer;
    const char *db_path = DEFAULT_DB_PATH;
    int status;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <customerusername> [database_path]\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (argc >= 3) {
        db_path = argv[2];
    }

    status = fetch_customer(db_path, argv[1], &customer);
    if (status == 0) {
        print_customer(&customer);
        return EXIT_SUCCESS;
    } else if (status == 1) {
        fprintf(stderr, "No customer found with username '%s'.\n", argv[1]);
        return EXIT_FAILURE;
    }

    return EXIT_FAILURE;
}