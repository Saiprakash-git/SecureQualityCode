#include <iostream>
#include <string>
#include <optional>
#include <sqlite3.h>

struct UserInfo {
    int id;
    std::string username;
    std::string email;
    std::string role;
};

bool initializeDatabase(sqlite3* db) {
    const char* createTableSql =
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "username TEXT UNIQUE NOT NULL, "
        "email TEXT NOT NULL, "
        "role TEXT NOT NULL);";

    char* errorMessage = nullptr;
    int rc = sqlite3_exec(db, createTableSql, nullptr, nullptr, &errorMessage);
    if (rc != SQLITE_OK) {
        if (errorMessage) {
            sqlite3_free(errorMessage);
        }
        return false;
    }

    const char* insertDataSql =
        "INSERT OR IGNORE INTO users (username, email, role) VALUES "
        "('alice', 'alice@example.com', 'Admin'), "
        "('bob', 'bob@example.com', 'User'), "
        "('charlie', 'charlie@example.com', 'Moderator');";

    rc = sqlite3_exec(db, insertDataSql, nullptr, nullptr, &errorMessage);
    if (rc != SQLITE_OK) {
        if (errorMessage) {
            sqlite3_free(errorMessage);
        }
        return false;
    }

    return true;
}

std::optional<UserInfo> getUserByUsername(sqlite3* db, const std::string& username) {
    if (!db) {
        return std::nullopt;
    }

    const char* querySql = "SELECT id, username, email, role FROM users WHERE username = ?;";
    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db, querySql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

    std::optional<UserInfo> result = std::nullopt;

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        UserInfo user;
        user.id = sqlite3_column_int(stmt, 0);

        const unsigned char* userText = sqlite3_column_text(stmt, 1);
        user.username = userText ? reinterpret_cast<const char*>(userText) : "";

        const unsigned char* emailText = sqlite3_column_text(stmt, 2);
        user.email = emailText ? reinterpret_cast<const char*>(emailText) : "";

        const unsigned char* roleText = sqlite3_column_text(stmt, 3);
        user.role = roleText ? reinterpret_cast<const char*>(roleText) : "";

        result = user;
    }

    sqlite3_finalize(stmt);
    return result;
}

int main() {
    sqlite3* db = nullptr;
    int rc = sqlite3_open(":memory:", &db);

    if (rc != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << "\n";
        sqlite3_close(db);
        return 1;
    }

    if (!initializeDatabase(db)) {
        std::cerr << "Failed to initialize database tables.\n";
        sqlite3_close(db);
        return 1;
    }

    std::cout << "Enter username to search (e.g., alice, bob, charlie): ";
    std::string searchUsername;
    if (std::cin >> searchUsername) {
        std::optional<UserInfo> user = getUserByUsername(db, searchUsername);
        if (user.has_value()) {
            std::cout << "\nUser Found:\n";
            std::cout << "ID:       " << user->id << "\n";
            std::cout << "Username: " << user->username << "\n";
            std::cout << "Email:    " << user->email << "\n";
            std::cout << "Role:     " << user->role << "\n";
        } else {
            std::cout << "\nUser not found: " << searchUsername << "\n";
        }
    }

    sqlite3_close(db);
    return 0;
}