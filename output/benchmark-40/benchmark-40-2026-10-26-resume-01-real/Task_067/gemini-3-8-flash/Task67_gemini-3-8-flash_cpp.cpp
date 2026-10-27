#include <iostream>
#include <string>
#include <optional>
#include <sqlite3.h>

struct Customer {
    int id;
    std::string username;
    std::string email;
    std::string fullName;
};

void initializeDatabaseIfEmpty(sqlite3* db) {
    const char* createTableQuery = 
        "CREATE TABLE IF NOT EXISTS customer ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "customerusername TEXT UNIQUE NOT NULL, "
        "email TEXT NOT NULL, "
        "fullname TEXT NOT NULL);";

    char* errorMessage = nullptr;
    sqlite3_exec(db, createTableQuery, nullptr, nullptr, &errorMessage);
    if (errorMessage) {
        sqlite3_free(errorMessage);
        return;
    }

    const char* seedQuery = 
        "INSERT OR IGNORE INTO customer (customerusername, email, fullname) "
        "VALUES ('jdoe', 'jdoe@example.com', 'John Doe'), "
        "       ('asmith', 'asmith@example.com', 'Alice Smith');";
    sqlite3_exec(db, seedQuery, nullptr, nullptr, &errorMessage);
    if (errorMessage) {
        sqlite3_free(errorMessage);
    }
}

std::optional<Customer> fetchCustomerByUsername(sqlite3* db, const std::string& username) {
    const char* sql = "SELECT id, customerusername, email, fullname FROM customer WHERE customerusername = ? LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    if (sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC) != SQLITE_OK) {
        sqlite3_finalize(stmt);
        return std::nullopt;
    }

    std::optional<Customer> customer;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        Customer c;
        c.id = sqlite3_column_int(stmt, 0);
        c.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        c.email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        c.fullName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        customer = c;
    }

    sqlite3_finalize(stmt);
    return customer;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <customerusername>" << std::endl;
        return 1;
    }

    std::string targetUsername = argv[1];

    sqlite3* db = nullptr;
    if (sqlite3_open("customer.db", &db) != SQLITE_OK) {
        std::cerr << "Error opening database: " << sqlite3_errmsg(db) << std::endl;
        if (db) sqlite3_close(db);
        return 1;
    }

    initializeDatabaseIfEmpty(db);

    std::optional<Customer> customer = fetchCustomerByUsername(db, targetUsername);

    if (customer.has_value()) {
        std::cout << "Customer Found:\n";
        std::cout << "ID:       " << customer->id << "\n";
        std::cout << "Username: " << customer->username << "\n";
        std::cout << "Email:    " << customer->email << "\n";
        std::cout << "Name:     " << customer->fullName << "\n";
    } else {
        std::cout << "Customer with username '" << targetUsername << "' not found." << std::endl;
    }

    sqlite3_close(db);
    return 0;
}