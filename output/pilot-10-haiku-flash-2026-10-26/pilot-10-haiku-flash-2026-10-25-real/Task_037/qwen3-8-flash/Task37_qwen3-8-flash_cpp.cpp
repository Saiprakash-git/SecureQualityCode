#include <iostream>
#include <string>
#include <sqlite3.h>

struct UserInfo {
    int id;
    std::string username;
    std::string email;
};

bool openDatabase(const std::string& databasePath, sqlite3** database) {
    if (database == nullptr) {
        return false;
    }

    int rc = sqlite3_open(databasePath.c_str(), database);
    if (rc != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(*database) << '\n';
        if (*database != nullptr) {
            sqlite3_close(*database);
        }
        *database = nullptr;
        return false;
    }

    return true;
}

bool ensureSchema(sqlite3* database) {
    if (database == nullptr) {
        return false;
    }

    const char* sql =
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "username TEXT UNIQUE NOT NULL, "
        "email TEXT NOT NULL"
        ");";

    char* errorMessage = nullptr;
    int rc = sqlite3_exec(database, sql, nullptr, nullptr, &errorMessage);

    if (rc != SQLITE_OK) {
        std::cerr << "Cannot create table: " << (errorMessage ? errorMessage : "unknown error") << '\n';
        if (errorMessage != nullptr) {
            sqlite3_free(errorMessage);
        }
        return false;
    }

    return true;
}

bool seedSampleUser(sqlite3* database) {
    if (database == nullptr) {
        return false;
    }

    const char* sql =
        "INSERT OR IGNORE INTO users (username, email) "
        "VALUES ('alice', 'alice@example.com');";

    char* errorMessage = nullptr;
    int rc = sqlite3_exec(database, sql, nullptr, nullptr, &errorMessage);

    if (rc != SQLITE_OK) {
        std::cerr << "Cannot seed sample user: " << (errorMessage ? errorMessage : "unknown error") << '\n';
        if (errorMessage != nullptr) {
            sqlite3_free(errorMessage);
        }
        return false;
    }

    return true;
}

bool fetchUserByUserName(sqlite3* database, const std::string& username, UserInfo& user) {
    if (database == nullptr || username.empty()) {
        return false;
    }

    const char* sql = "SELECT id, username, email FROM users WHERE username = ?;";
    sqlite3_stmt* statement = nullptr;

    int rc = sqlite3_prepare_v2(database, sql, -1, &statement, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Cannot prepare query: " << sqlite3_errmsg(database) << '\n';
        return false;
    }

    rc = sqlite3_bind_text(statement, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) {
        std::cerr << "Cannot bind username: " << sqlite3_errmsg(database) << '\n';
        sqlite3_finalize(statement);
        return false;
    }

    rc = sqlite3_step(statement);

    if (rc == SQLITE_ROW) {
        user.id = sqlite3_column_int(statement, 0);

        const unsigned char* rawUsername = sqlite3_column_text(statement, 1);
        const unsigned char* rawEmail = sqlite3_column_text(statement, 2);

        user.username = rawUsername ? reinterpret_cast<const char*>(rawUsername) : "";
        user.email = rawEmail ? reinterpret_cast<const char*>(rawEmail) : "";

        sqlite3_finalize(statement);
        return true;
    }

    sqlite3_finalize(statement);

    if (rc != SQLITE_DONE) {
        std::cerr << "Cannot execute query: " << sqlite3_errmsg(database) << '\n';
        return false;
    }

    return false;
}

void closeDatabase(sqlite3* database) {
    if (database != nullptr) {
        sqlite3_close(database);
    }
}

int main() {
    const std::string databasePath = "users.db";
    sqlite3* database = nullptr;

    if (!openDatabase(databasePath, &database)) {
        return 1;
    }

    if (!ensureSchema(database)) {
        closeDatabase(database);
        return 1;
    }

    if (!seedSampleUser(database)) {
        closeDatabase(database);
        return 1;
    }

    std::string username;
    std::cout << "Enter username: ";
    std::getline(std::cin, username);

    if (username.empty()) {
        std::cerr << "Username cannot be empty.\n";
        closeDatabase(database);
        return 1;
    }

    UserInfo user;
    if (fetchUserByUserName(database, username, user)) {
        std::cout << "User ID: " << user.id << '\n';
        std::cout << "Username: " << user.username << '\n';
        std::cout << "Email: " << user.email << '\n';
    } else {
        std::cout << "No user found for username: " << username << '\n';
    }

    closeDatabase(database);
    return 0;
}