import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.SQLException;
import java.util.Properties;

public class Main {
    public static Connection createConnection(
            String host,
            int port,
            String database,
            String username,
            String password) throws SQLException {

        if (host == null || host.isEmpty()) {
            throw new SQLException("Host is required.");
        }

        if (port <= 0 || port > 65535) {
            throw new SQLException("Invalid port.");
        }

        if (database == null || database.isEmpty()) {
            throw new SQLException("Database is required.");
        }

        if (username == null) {
            throw new SQLException("Username is required.");
        }

        try {
            Class.forName("org.postgresql.Driver");
        } catch (ClassNotFoundException e) {
            throw new SQLException("PostgreSQL JDBC driver not found.", e);
        }

        String url = "jdbc:postgresql://" + host + ":" + port + "/" + database;

        Properties properties = new Properties();
        properties.setProperty("user", username);
        properties.setProperty("password", password == null ? "" : password);

        return DriverManager.getConnection(url, properties);
    }
}