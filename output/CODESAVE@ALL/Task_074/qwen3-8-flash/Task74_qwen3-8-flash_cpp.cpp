#include <libpq-fe.h>
#include <stdexcept>
#include <string>

namespace {
    std::string safeText(const char* text) {
        return text ? std::string(text) : std::string();
    }
}

class PostgresConnection {
public:
    explicit PostgresConnection(PGconn* conn) : conn_(conn) {
        if (conn_ == nullptr) {
            throw std::runtime_error("Failed to allocate PostgreSQL connection object.");
        }

        if (PQstatus(conn_) != CONNECTION_OK) {
            std::string message = safeText(PQerrorMessage(conn_));
            PQfinish(conn_);
            conn_ = nullptr;
            throw std::runtime_error("PostgreSQL connection failed: " + message);
        }
    }

    PostgresConnection(const PostgresConnection&) = delete;
    PostgresConnection& operator=(const PostgresConnection&) = delete;

    PostgresConnection(PostgresConnection&& other) noexcept : conn_(other.conn_) {
        other.conn_ = nullptr;
    }

    PostgresConnection& operator=(PostgresConnection&& other) noexcept {
        if (this != &other) {
            if (conn