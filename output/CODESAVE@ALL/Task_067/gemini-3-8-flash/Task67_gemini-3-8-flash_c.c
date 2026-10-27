#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

typedef struct {
    int id;
    char username[64];
    char full_name[128];
    char email[128];
    char phone[32];
    int found;
} Customer;

int setup_database(sqlite3 *db) {
    const char *create_sql = 
        "CREATE TABLE IF NOT EXISTS customer ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "customerusername TEXT UNIQUE NOT NULL,"
        "full_name TEXT NOT NULL,"
        "email TEXT NOT NULL,"
        "phone TEXT NOT NULL"
        ");";

    char *err_msg = NULL;
    int rc = sqlite3_exec(db, create_sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return 0;
    }

    const char *seed_sql = 
        "INSERT OR IGNORE INTO customer (customerusername, full_name, email, phone) "
        "VALUES ('jdoe', 'John Doe', 'jdoe@example.com', '555-0101'),"
        "       ('asmith', 'Alice Smith', 'asmith@example.com', '555-0102'),"
        "       ('bwayne', 'Bruce Wayne', 'bwayne@wayneenterprises.com', '555-0103');";

    rc = sqlite3_exec(db, seed_sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return 0;
    }

    return 1;
}

Customer get_customer_by_username(sqlite3 *db, const char *username) {
    Customer cust;
    memset(&cust, 0, sizeof(Customer));
    cust.found = 0;

    if (!db || !username) {
        return cust;
    }

    const char *query = "SELECT id, customerusername, full_name, email, phone FROM customer WHERE customerusername = ? LIMIT 1;";
    sqlite3_stmt *stmt = NULL;

    int rc = sqlite3_prepare_v2(db, query, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return cust;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        cust.id = sqlite3_column_int(stmt, 0);

        const unsigned char *uname = sqlite3_column_text(stmt, 1);
        if (uname) strncpy(cust.username, (const char *)uname, sizeof(cust.username) - 1);

        const unsigned char *name = sqlite3_column_text(stmt, 2);
        if (name) strncpy(cust.full_name, (const char *)name, sizeof(cust.full_name) - 1);

        const unsigned char *email = sqlite3_column_text(stmt, 3);
        if (email) strncpy(cust.email, (const char *)email, sizeof(cust.email) - 1);

        const unsigned char *phone = sqlite3_column_text(stmt, 4);
        if (phone) strncpy(cust.phone, (const char *)phone, sizeof(cust.phone) - 1);

        cust.found = 1;
    }

    sqlite3_finalize(stmt);
    return cust;
}

void display_customer(const Customer *cust) {
    if (!cust) return;

    if (cust->found) {
        printf("Customer Details Found:\n");
        printf("------------------------\n");
        printf("ID:       %d\n", cust->id);
        printf("Username: %s\n", cust->username);
        printf("Name:     %s\n", cust->full_name);
        printf("Email:    %s\n", cust->email);
        printf("Phone:    %s\n", cust->phone);
    } else {
        printf("Customer not found.\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <customerusername>\n", argv[0]);
        return EXIT_FAILURE;
    }

    sqlite3 *db = NULL;
    int rc = sqlite3_open("customer.db", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return EXIT_FAILURE;
    }

    if (!setup_database(db)) {
        sqlite3_close(db);
        return EXIT_FAILURE;
    }

    const char *target_username = argv[1];
    Customer customer = get_customer_by_username(db, target_username);

    display_customer(&customer);

    sqlite3_close(db);
    return customer.found ? EXIT_SUCCESS : EXIT_FAILURE;
}