#include <iostream>
#include <string>
#include <optional>
#include <sqlite3.h>

struct Customer {
    std::string customerUsername;
    std::string firstName;
    std::string lastName;
    std::string email;
    std::string phone;
    std::string address;
};

static std::string columnText(sqlite3_stmt* stmt, int index) {
    const unsigned char* text = sqlite3_column_text(stmt, index);
    return text ? std::string(reinterpret_cast<const char*>(text)) : std::string();
}

std::optional<Customer> fetchCustomerByUsername(sqlite3* db, const std::string& username) {
    const char* sql =
        "SELECT customerusername, first_name, last_name, email, phone, address "
        "FROM customer WHERE customerusername = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << "\n";
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), static_cast<int>(username.size()), SQLITE_TRANSIENT);

    std::optional<Customer> result;
    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        Customer c;
        c.customerUsername = columnText(stmt, 0);
        c.firstName        = columnText(stmt, 1);
        c.lastName         = columnText(stmt, 2);
        c.email            = columnText(stmt, 3);
        c.phone            = columnText(stmt, 4);
        c.address          = columnText(stmt, 5);
        result = c;
    } else if (rc != SQLITE_DONE) {
        std::cerr << "Query execution error: " << sqlite3_errmsg(db) << "\n";
    }

    sqlite3_finalize(stmt);
    return result;
}

void printCustomer(const Customer& c) {
    std::cout << "Username : " << c.customerUsername << "\n"
              << "First    : " << c.firstName << "\n"
              << "Last     : " << c.lastName << "\n"
              << "Email    : " << c.email << "\n"
              << "Phone    : " << c.phone << "\n"
              << "Address  : " << c.address << "\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <customerusername> [database_path]\n";
        return 1;
    }

    std::string username = argv[1];
    std::string dbPath = (argc >= 3) ? argv[2] : "database.db";

    sqlite3* db = nullptr;
    if (sqlite3_open(dbPath.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Cannot open database '" << dbPath << "': " << sqlite3_errmsg(db) << "\n";
        sqlite3_close(db);
        return 1;
    }

    std::optional<Customer> customer = fetchCustomerByUsername(db, username);
    sqlite3_close(db);

    if (!customer) {
        std::cout << "No customer found with username '" << username << "'.\n";
        return 2;
    }

    printCustomer(*customer);
    return 0;
}