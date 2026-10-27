#include <sqlite3.h>

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

std::string jsonEscape(const std::string& value) {
    std::ostringstream out;
    for (unsigned char ch : value) {
        switch (ch) {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\b': out << "\\b"; break;
            case '\f': out << "\\f"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:
                if (ch < 0x20) {
                    const char hex[] = "0123456789abcdef";
                    out << "\\u00" << hex[ch >> 4] << hex[ch & 0x0f];
                } else {
                    out << static_cast<char>(ch);
                }
        }
    }
    return out.str();
}

std::string base64Encode(const unsigned char* data, int length) {
    static const char alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    result.reserve(((length + 2) / 3) * 4);

    for (int i = 0; i < length; i += 3) {
        unsigned int value = static_cast<unsigned int>(data[i]) << 16;
        if (i + 1 < length) value |= static_cast<unsigned int>(data[i + 1]) << 8;
        if (i + 2 < length) value |= static_cast<unsigned int>(data[i + 2]);

        result.push_back(alphabet[(value >> 18) & 0x3f]);
        result.push_back(alphabet[(value >> 12) & 0x3f]);
        result.push_back(i + 1 < length ? alphabet[(value >> 6) & 0x3f] : '=');
        result.push_back(i + 2 < length ? alphabet[value & 0x3f] : '=');
    }
    return result;
}

std::string columnValueAsJson(sqlite3_stmt* statement, int column) {
    switch (sqlite3_column_type(statement, column)) {
        case SQLITE_NULL:
            return "null";
        case SQLITE_INTEGER:
        case SQLITE_FLOAT: {
            const unsigned char* value = sqlite3_column_text(statement, column);
            return value ? reinterpret_cast<const char*>(value) : "null";
        }
        case SQLITE_BLOB: {
            const auto* data =
                static_cast<const unsigned char*>(sqlite3_column_blob(statement, column));
            const int length = sqlite3_column_bytes(statement, column);
            return "\"" + base64Encode(data, length) + "\"";
        }
        case SQLITE_TEXT:
        default: {
            const unsigned char* value = sqlite3_column_text(statement, column);
            const int length = sqlite3_column_bytes(statement, column);
            return "\"" + jsonEscape(
                value ? std::string(reinterpret_cast<const char*>(value), length) : "") + "\"";
        }
    }
}

bool fetchCustomer(const std::string& databasePath,
                   const std::string& customerUsername,
                   std::string& result,
                   std::string& error) {
    sqlite3* database = nullptr;
    if (sqlite3_open_v2(databasePath.c_str(), &database, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK) {
        error = database ? sqlite3_errmsg(database) : "Unable to open database";
        if (database) sqlite3_close(database);
        return false;
    }

    const char* sql = "SELECT * FROM customer WHERE customerusername = ? LIMIT 1";
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(database, sql, -1, &statement, nullptr) != SQLITE_OK) {
        error = sqlite3_errmsg(database);
        sqlite3_close(database);
        return false;
    }

    if (sqlite3_bind_text(statement, 1, customerUsername.c_str(),
                          static_cast<int>(customerUsername.size()), SQLITE_TRANSIENT) != SQLITE_OK) {
        error = sqlite3_errmsg(database);
        sqlite3_finalize(statement);
        sqlite3_close(database);
        return false;
    }

    const int stepResult = sqlite3_step(statement);
    if (stepResult == SQLITE_DONE) {
        result = "null";
    } else if (stepResult != SQLITE_ROW) {
        error = sqlite3_errmsg(database);
        sqlite3_finalize(statement);
        sqlite3_close(database);
        return false;
    } else {
        std::ostringstream json;
        json << '{';
        const int columnCount = sqlite3_column_count(statement);
        for (int column = 0; column < columnCount; ++column) {
            if (column > 0) json << ',';
            const char* name = sqlite3_column_name(statement, column);
            json << '"' << jsonEscape(name ? name : "") << "\":"
                 << columnValueAsJson(statement, column);
        }
        json << '}';
        result = json.str();
    }

    sqlite3_finalize(statement);
    sqlite3_close(database);
    return true;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <customerusername> [database_path]\n";
        return 1;
    }

    const char* configuredPath = std::getenv("CUSTOMER_DB");
    const std::string databasePath =
        argc >= 3 ? argv[2] : (configuredPath ? configuredPath : "customer.db");

    std::string result;
    std::string error;
    if (!fetchCustomer(databasePath, argv[1], result, error)) {
        std::cerr << "Database error: " << error << '\n';
        return 1;
    }

    std::cout << result << '\n';
    return 0;
}