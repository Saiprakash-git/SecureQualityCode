#include <iostream>
#include <string>
#include <optional>
#include <sqlite3.h>

struct Customer {
    int id;
    std::string username;
    std::string email;
    std::string firstName;
    std::string lastName;
    std::string phone;
    std::string address;
    std::string createdAt;
};

std::optional<Customer> getCustomerByUsername(const std::string& dbPath, const std::string& customerUsername) {
    sqlite3* db = nullptr;
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_open(dbPath.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        return std::nullopt;
    }

    const char* query = "SELECT id, username, email, first_name, last_name, phone, address, created_at FROM customer WHERE username = ? LIMIT 1";

    if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, customerUsername.c_str(), -1, SQLITE_TRANSIENT);

    std::optional<Customer> result;

    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        auto getText = [](sqlite3_stmt* s, int col) -> std::string {
            const unsigned char* text = sqlite3_column_text(s, col);
            return text ? reinterpret_cast<const char*>(text) : "";
        };

        Customer c;
        c.id = sqlite3_column_int(stmt, 0);
        c.username = getText(stmt, 1);
        c.email = getText(stmt, 2);
        c.firstName = getText(stmt, 3);
        c.lastName = getText(stmt, 4);
        c.phone = getText(stmt, 5);
        c.address = getText(stmt, 6);
        c.createdAt = getText(stmt, 7);
        result = c;
    } else if (rc != SQLITE_DONE) {
        std::cerr << "Execution failed: " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return result;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <customerusername>" << std::endl;
        return 1;
    }

    std::string customerUsername = argv[1];
    std::string dbPath = "customers.db";

    auto customer = getCustomerByUsername(dbPath, customerUsername);

    if (customer.has_value()) {
        std::cout << "{\n";
        std::cout << "  \"id\": " << customer->id << ",\n";
        std::cout << "  \"username\": \"" << customer->username << "\",\n";
        std::cout << "  \"email\": \"" << customer->email << "\",\n";
        std::cout << "  \"firstName\": \"" << customer->firstName << "\",\n";
        std::cout << "  \"lastName\": \"" << customer->lastName << "\",\n";
        std::cout << "  \"phone\": \"" << customer->phone << "\",\n";
        std::cout << "  \"address\": \"" << customer->address << "\",\n";
        std::cout << "  \"createdAt\": \"" << customer->createdAt << "\"\n";
        std::cout << "}" << std::endl;
        return 0;
    } else {