#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

typedef struct {
    int id;
    char customerusername[256];
    char name[256];
    char email[256];
    char phone[64];
} Customer;

int get_customer_by_username(const char* db_path, const char* username, Customer* customer) {
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;
    int rc;

    rc = sqlite3_open(db_path, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    const char* sql = "SELECT id, customerusername, name, email, phone FROM customer WHERE customerusername = ? LIMIT 1;";

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return -1;
    }

    rc = sqlite3_bind_text(stmt, 1, username, -1, SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to bind parameter: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return -1;
    }

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        const unsigned char* text;

        customer->id = sqlite3_column_int(stmt, 0);

        text = sqlite3_column_text(stmt, 1);
        if (text != NULL) {
            strncpy(customer->customerusername, (const char*)text, sizeof(customer->customerusername) - 1);
            customer->customerusername[sizeof(customer->customerusername) - 1] = '\0';
        } else {
            customer->customerusername[0] = '\0';
        }

        text = sqlite3_column_text(stmt, 2);
        if (text != NULL) {
            strncpy(customer->name, (const char*)text, sizeof(customer->name) - 1);
            customer->name[sizeof(customer->name) - 1] = '\0';
        } else {
            customer->name[0] = '\0';
        }

        text = sqlite3_column_text(stmt, 3);
        if (text != NULL) {
            strncpy(customer->email, (const char*)text, sizeof(customer->email) - 1);
            customer->email[sizeof(customer->email) - 1] = '\0';
        } else {
            customer->email[0] = '\0';
        }

        text = sqlite3_column_text(stmt, 4);
        if (text != NULL) {
            strncpy(customer->phone, (const char*)text, sizeof(customer->phone) - 1);
            customer->phone[sizeof(customer->phone) - 1] = '\0';
        } else {
            customer->phone[0] = '\0';
        }

        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 0;
    } else if (rc == SQLITE_DONE) {
        fprintf(stderr, "Customer not found: %s\n", username);
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;