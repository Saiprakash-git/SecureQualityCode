#include <iostream>
#include <memory>
#include <string>
#include <libpq-fe.h>

class PostgresConnection {
public:
    explicit PostgresConnection(PGconn* conn)
        : conn_(conn, [](PGconn* c) { if (c) PQfinish(c); }) {}

    bool isValid() const {
        return conn_ && (PQstatus(conn_.get()) == CONNECTION_OK);
    }

    std::string getErrorMessage() const {
        return conn_ ? PQerrorMessage(conn_.get()) : "Invalid connection handle.";
    }

    PGconn* getRawHandle() const {
        return conn_.get();
    }

private:
    std::unique_ptr<PGconn, void(*)(PGconn*)> conn_;
};

std::unique_ptr<PostgresConnection> createPostgresConnection(
    const std::string& host,
    int port,
    const std::string& dbname,
    const std::string& user,
    const std::string& password) {

    std::string conninfo = "host=" + host +
                           " port=" + std::to_string(port) +
                           " dbname=" + dbname +
                           " user=" + user +
                           " password=" + password;

    PGconn* rawConn = PQconnectdb(conninfo.c_str());
    auto connection = std::make_unique<PostgresConnection>(rawConn);

    if (!connection->isValid()) {
        std::cerr << "Failed to connect to database: " << connection->getErrorMessage() << std::endl;
        return nullptr;
    }

    return connection;
}

std::unique_ptr<PostgresConnection> createPostgresConnection(const std::string& conninfo) {
    PGconn* rawConn = PQconnectdb(conninfo.c_str());
    auto connection = std::make_unique<PostgresConnection>(rawConn);

    if (!connection->isValid()) {
        std::cerr << "Failed to connect to database: " << connection->getErrorMessage() << std::endl;
        return nullptr;
    }

    return connection;
}

int main() {
    std::string host = "localhost";
    int port = 5432;
    std::string dbname = "testdb";
    std::string user = "postgres";
    std::string password = "password";

    std::unique_ptr<PostgresConnection> conn = createPostgresConnection(host, port, dbname, user, password);

    if (conn && conn->isValid()) {
        std::cout << "Successfully connected to PostgreSQL database: " << dbname << std::endl;
    } else {
        std::cout << "Database connection attempt failed." << std::endl;
    }

    return 0;
}