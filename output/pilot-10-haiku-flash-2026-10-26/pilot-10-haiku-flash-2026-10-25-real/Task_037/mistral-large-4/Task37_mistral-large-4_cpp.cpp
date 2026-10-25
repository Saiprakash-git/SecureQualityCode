#include <iostream>
#include <string>
#include <vector>
#include <sqlite3.h>

struct UserInfo {
    int id;
    std::string username;
    std::string email;
    std::string fullName;
};

sqlite3* connectDatabase(const std::string& dbPath) {
    sqlite3* db = nullptr;
    int rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return nullptr;
    }
    return db;
}

std::vector<UserInfo> getUserByUsername(sqlite3* db, const std::string& username) {
    std::vector<UserInfo> users;
    const char* sql = "SELECT id, username, email, full_name FROM users WHERE username = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return users;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        UserInfo user;
        user.id = sqlite3_column_int(stmt, 0);

        const unsigned char* username_col = sqlite3_column_text(stmt, 1);
        user.username = username_col ? reinterpret_cast<const char*>(username_col) : "";

        const unsigned char* email_col = sqlite3_column_text(stmt, 2);
        user.email = email_col ? reinterpret_cast<const char*>(email_col) : "";

        const unsigned char* name_col = sqlite3_column_text(stmt, 3);
        user.fullName = name_col ? reinterpret_cast<const char*>(name_col) : "";

        users.push_back(user);
    }

    sqlite3_finalize(stmt);
    return users;
}

void displayUsers(const std::vector<UserInfo>& users) {
    if (users.empty()) {
        std::cout << "No users found." << std::endl;
        return;
    }
    for (const auto& user : users) {
        std::cout << "ID: " << user.id << ", Username: " << user.username
                  << ", Email: " << user.email << ", Name: " << user.fullName << std::endl;
    }
}

int main() {
    std::string dbPath = "users.db";
    std::string username;

    std::cout << "Enter username: ";
    std::getline(std::cin, username);

    sqlite3* db = connectDatabase(dbPath);
    if (!db) {
        return 1;
    }

    std::vector<UserInfo> users = getUserByUsername(db, username);
    displayUsers(users);

    sqlite3_close(db);
    return 0;
}