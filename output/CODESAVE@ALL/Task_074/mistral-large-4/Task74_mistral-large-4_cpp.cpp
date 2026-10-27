#include <libpq-fe.h>
#include <string>
#include <stdexcept>
#include <utility>

class PostgresConnection {
public:
    explicit PostgresConnection(PGconn* conn) : conn_(conn) {
        if (!conn_) {
            throw std::runtime_error("PQconnectdb returned null");
        }
        if (PQstatus(conn_) != CONNECTION_OK) {
            std::string msg = PQerrorMessage(conn_);
            PQfinish(conn_);
            conn_ = nullptr;
            throw std::runtime_error("Connection failed: " + msg);
        }
    }

    ~PostgresConnection() {
        if (conn_) {
            PQfinish(conn_);
        }
    }

    PostgresConnection(const PostgresConnection&) = delete;
    PostgresConnection& operator=(const PostgresConnection&) = delete;

    PostgresConnection(PostgresConnection&& other) noexcept
        : conn_(std::exchange(other.conn_, nullptr)) {}

    PostgresConnection& operator=(PostgresConnection&& other) noexcept {
        if (this != &other) {
            if (conn_) {
                PQfinish(conn_);
            }
            conn_ = std::exchange(other.conn_, nullptr);
        }
        return *this;
    }

    PGconn* get() const noexcept { return conn_; }
    bool isValid() const noexcept { return conn_ && PQstatus(conn_) == CONNECTION_OK; }

private:
    PGconn* conn_;
};

PostgresConnection createPostgresConnection(const std::string& host,
                                           const std::string& port,
                                           const std::string& dbname,
                                           const std::string& user,
                                           const std::string& password) {
    std::string conninfo = "host=" + host +
                          " port=" + port +
                          " dbname=" + dbname +
                          " user=" + user +
                          " password=" + password;
    return PostgresConnection(PQconnectdb(conninfo.c_str()));
}