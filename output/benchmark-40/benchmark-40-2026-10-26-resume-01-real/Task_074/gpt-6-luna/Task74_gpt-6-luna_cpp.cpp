#include <libpq-fe.h>

#include <memory>
#include <stdexcept>
#include <string>

struct PostgresConnectionDeleter {
    void operator()(PGconn* connection) const noexcept {
        if (connection != nullptr) {
            PQfinish(connection);
        }
    }
};

using PostgresConnection = std::unique_ptr<PGconn, PostgresConnectionDeleter>;

PostgresConnection createPostgresConnection(const std::string& connectionInfo) {
    PostgresConnection connection(PQconnectdb(connectionInfo.c_str()));

    if (!connection) {
        throw std::runtime_error("Failed to allocate a PostgreSQL connection.");
    }

    if (PQstatus(connection.get()) != CONNECTION_OK) {
        const char* error = PQerrorMessage(connection.get());
        throw std::runtime_error(
            error != nullptr ? error : "Failed to connect to PostgreSQL.");
    }

    return connection;
}