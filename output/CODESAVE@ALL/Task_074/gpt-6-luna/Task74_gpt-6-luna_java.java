import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.SQLException;
import java.util.Properties;

public final class PostgresConnection {
    private PostgresConnection() {
    }

    public static Connection createConnection(
            String host,
            int port,
            String database,
            String username,
            String password) throws SQLException {
        if (host == null || host.isBlank()) {
            throw new IllegalArgumentException("Host must not be blank.");
        }
        if (port < 1 || port > 65535) {
            throw new IllegalArgumentException("Port must be between 1 and 65535.");
        }
        if (database == null || database.isBlank()) {
            throw new IllegalArgumentException("Database must not be blank.");
        }
        if (username == null || username.isBlank()) {
            throw new IllegalArgumentException("Username must not be blank.");
        }

        String url = "jdbc:postgresql://" + host + ":" + port + "/" + database;
        Properties properties = new Properties();
        properties.setProperty("user", username);

        if (password != null) {
            properties.setProperty("password", password);
        }

        return DriverManager.getConnection(url, properties);
    }
}