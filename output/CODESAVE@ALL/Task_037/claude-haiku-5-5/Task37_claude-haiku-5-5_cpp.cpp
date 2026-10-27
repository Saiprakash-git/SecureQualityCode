```cpp
#include <iostream>
#include <string>
#include <optional>
#include <sqlite3.h>

struct UserInfo {
    int id;
    std::string username;
    std::string email;
    std::string full_name;
    std::string created_at;
};

bool openDatabase(const std::string& path, sqlite3*& db, std::string& error) {
    if (sqlite3_open(path.c_str(), &db) != SQLITE_OK) {
        error = sqlite3_errmsg(db);
        sqlite3_close(db);
        db = nullptr;
        return false;
    }
    return true;
}

bool initializeSchema(sqlite3* db, std::string& error) {
    const char* sql =
        "CREATE TABLE IF NOT EXISTS users ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  username TEXT NOT NULL UNIQUE,"
        "  email TEXT NOT NULL,"
        "  full_name TEXT,"
        "  created_at TEXT DEFAULT CURRENT_TIMESTAMP"
        ");";
    char* errMsg = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        error = errMsg ? errMsg : "unknown error";
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool insertSampleUser(sqlite3* db, const std::string& username,
                      const std::string& email, const std::string& fullName,
                      std::string& error) {
    const char* sql =
        "INSERT OR IGNORE INTO users (username, email, full_name) VALUES (?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        error = sqlite3_errmsg(db);
        return false;
    }
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, email.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, fullName.c_str(), -1, SQLITE_TRANSIENT);

    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    if (!ok) error = sqlite3_errmsg(db);
    sqlite3_finalize(stmt);
    return ok;
}

std::optional<UserInfo> fetchUserByUsername(sqlite3* db, const std::string& username,
                                            std::string& error) {
    const char* sql =
        "SELECT id, username, email, COALESCE(full_name, ''), COALESCE(created_at, '') "
        "FROM users WHERE username = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        error = sqlite3_errmsg(db);
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

    std::optional<UserInfo> result;
    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        UserInfo user;
        user.id = sqlite3_column_int(stmt, 0);
        user.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        user.email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        user.full_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        user.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        result = user;
    } else if (rc != SQLITE_DONE) {
        error = sqlite3_errmsg(db);
    }

    sqlite3_finalize(stmt);
    return result;
}

void printUser(const UserInfo& user) {
    std::cout << "ID:         " << user.id << "\n"
              << "Username:   " << user.username << "\n"
              << "Email:      " << user.email << "\n"
              << "Full name:  " << user.full_name << "\n"
              << "Created at: " << user.created_at << "\n";
}

std::string readUsername(int argc, char* argv[]) {
    if (argc > 2) {
        return argv[2];
    }
    std::cout << "Enter username: ";
    std::string username;
    std::getline(std::cin, username);
    return username;
}

int main(int argc, char* argv[]) {
    std::string dbPath = (argc > 1) ? argv[1] : "users.db";
    std::string error;

    sqlite3* db = nullptr;
    if (!openDatabase(dbPath, db, error)) {
        std::cerr << "Failed to open database: " << error << "\n";
        return 1;
    }

    if (!initializeSchema(db, error)) {
        std::cerr << "Failed to initialize schema: " << error << "\n";
        sqlite3_close(db);
        return 1;
    }

    insertSampleUser(db, "alice", "alice@example.com", "Alice Smith", error);
    insertSampleUser(db, "bob", "bob@example.com", "Bob Jones", error);
    error.clear();

    std::string username = readUsername(argc, argv);
    if (username.empty()) {
        std::cerr << "Username cannot be empty.\n";
        sqlite3_close(db);
        return 1;
    }

    auto user = fetchUserByUsername(db, username, error);
    if (!error.empty()) {
        std::cerr << "Query failed: " << error << "\n";
        sqlite3_close(db);
        return 1;
    }

    if (user) {
        printUser(*user);
    } else {
        std::cout << "No user found with username '" << username << "'.\n";
    }

    sqlite3_close(db);
    return 0;
}
```