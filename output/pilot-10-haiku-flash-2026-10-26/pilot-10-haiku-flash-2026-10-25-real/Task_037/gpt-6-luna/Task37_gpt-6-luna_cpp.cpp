#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <sqlite3.h>

struct User {
    std::int64_t id;
    std::string username;
    std::string email;
    std::string fullName;
};

std::string columnText(sqlite3_stmt* statement, int column) {
    const unsigned char* value = sqlite3_column_text(statement, column);
    return value ? reinterpret_cast<const char*>(value) : "";
}

std::optional<User> findUser(sqlite3* database, const std::string& username) {
    const char* sql =
        "SELECT id, username, email, full_name "
        "FROM users WHERE username = ? LIMIT 1";

    sqlite3_stmt* statement = nullptr;
    int result = sqlite3_prepare_v2(database, sql, -1, &statement, nullptr);
    if (result != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }

    result = sqlite3_bind_text(statement, 1, username.c_str(),
                               static_cast<int>(username.size()), SQLITE_TRANSIENT);
    if (result != SQLITE_OK) {
        std::string error = sqlite3_errmsg(database);
        sqlite3_finalize(statement);
        throw std::runtime_error(error);
    }

    result = sqlite3_step(statement);
    if (result == SQLITE_ROW) {
        User user{
            sqlite3_column_int64(statement, 0),
            columnText(statement, 1),
            columnText(statement, 2),
            columnText(statement, 3)
        };
        sqlite3_finalize(statement);
        return user;
    }

    if (result != SQLITE_DONE) {
        std::string error = sqlite3_errmsg(database);
        sqlite3_finalize(statement);
        throw std::runtime_error(error);
    }

    sqlite3_finalize(statement);
    return std::nullopt;
}

int main(int argc, char* argv[]) {
    const std::string databasePath = argc > 1 ? argv[1] : "users.db";

    std::string username;
    if (argc > 2) {
        username = argv[2];
    } else {
        std::cout << "Enter username: ";
        if (!std::getline(std::cin, username)) {
            std::cerr << "Failed to read username.\n";
            return 1;
        }
    }

    sqlite3* database = nullptr;
    int result = sqlite3_open(databasePath.c_str(), &database);
    if (result != SQLITE_OK) {
        std::cerr << "Unable to open database: "
                  << (database ? sqlite3_errmsg(database) : "unknown error")
                  << '\n';
        if (database) {
            sqlite3_close(database);
        }
        return 1;
    }

    try {
        const std::optional<User> user = findUser(database, username);
        if (user) {
            std::cout << "ID: " << user->id << '\n'
                      << "Username: " << user->username << '\n'
                      << "Email: " << user->email << '\n'
                      << "Full name: " << user->fullName << '\n';
        } else {
            std::cout << "User not found.\n";
        }
    } catch (const std::exception& error) {
        std::cerr << "Database query failed: " << error.what() << '\n';
        sqlite3_close(database);
        return 1;
    }

    sqlite3_close(database);
    return 0;
}