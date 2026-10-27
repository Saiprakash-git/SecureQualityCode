#include <libpq-fe.h>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

struct ConnectionParams {
    std::string host = "localhost";
    std::string port = "5432";
    std::string dbname = "postgres";
    std::string user = "postgres";
    std::string password;
    int connect_timeout = 10;
    std::string sslmode = "prefer";
};

using PgConnPtr = std::unique_ptr<PGconn, void (*)(PGconn*)>;

std::string quote_conninfo_value(const std::string& value) {
    std::string out = "'";
    for (char c : value) {
        if (c == '\\' || c == '\'') {
            out += '\\';
        }
        out += c;
    }
    out += '\'';
    return out;
}

std::string build_conninfo(const ConnectionParams& p) {
    std::ostringstream oss;
    oss << "host=" << quote_conninfo_value(p.host)
        << " port=" << quote_conninfo_value(p.port)
        << " dbname=" << quote_conninfo_value(p.dbname)
        << " user=" << quote_conninfo_value(p.user)
        << " password=" << quote_conninfo_value(p.password)
        << " connect_timeout=" << p.connect_timeout
        << " sslmode=" << quote_conninfo_value(p.sslmode);
    return oss.str();
}

PgConnPtr create_connection(const ConnectionParams& params) {
    const std::string conninfo = build_conninfo(params);

    PGconn* raw = PQconnectdb(conninfo.c_str());
    if (raw == nullptr) {
        throw std::runtime_error("Failed to allocate PostgreSQL connection object");
    }

    PgConnPtr conn(raw, &PQfinish);

    if (PQstatus(conn.get()) != CONNECTION_OK) {
        std::string err = PQerrorMessage(conn.get());
        throw std::runtime_error("PostgreSQL connection failed: " + err);
    }

    return conn;
}

bool check_connection(PGconn* conn, std::string& server_version) {
    if (conn == nullptr) {
        return false;
    }

    PGresult* res = PQexec(conn, "SELECT version()");
    if (res == nullptr) {
        return false;
    }

    bool ok = PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0;
    if (ok) {
        server_version = PQgetvalue(res, 0, 0);
    }
    PQclear(res);
    return ok;
}

ConnectionParams parse_params(int argc, char* argv[]) {
    ConnectionParams p;
    if (const char* env = std::getenv("PGPASSWORD")) {
        p.password = env;
    }
    if (argc > 1) p.host = argv[1];
    if (argc > 2) p.port = argv[2];
    if (argc > 3) p.dbname = argv[3];
    if (argc > 4) p.user = argv[4];
    if (argc > 5) p.password = argv[5];
    return p;
}

int main(int argc, char* argv[]) {
    try {
        ConnectionParams params = parse_params(argc, argv);

        PgConnPtr conn = create_connection(params);

        std::string version;
        if (check_connection(conn.get(), version)) {
            std::cout << "Connected successfully.\n";
            std::cout << "Server: " << version << "\n";
        } else {
            std::cerr << "Connected, but health check query failed.\n";
            return 1;
        }

        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }
}