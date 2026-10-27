import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.SQLException;
import java.util.Properties;

public class PostgresConnector {

    public static Connection createConnection(String host, int port, String database, String user, String password) throws SQLException {
        String url = String.format("jdbc:postgresql://%s:%d/%s", host, port, database);
        return DriverManager.getConnection(url, user, password);
    }

    public static Connection createConnection(String host, int port, String database, Properties properties) throws SQLException {
        String url = String.format("jdbc:postgresql://%s:%d/%s", host, port, database);
        return DriverManager.getConnection(url, properties);
    }

    public static Connection createConnection(String jdbcUrl, String user, String password) throws SQLException {
        return DriverManager.getConnection(jdbcUrl, user, password);
    }

    public static void main(String[] args) {
        String host = "localhost";
        int port = 5432;
        String database = "testdb";
        String user = "postgres";
        String password = "secretpassword";

        try (Connection conn = createConnection(host, port, database, user, password)) {
            if (conn != null && !conn.isClosed()) {
                System.out.println("Connection established successfully: " + conn.getMetaData().getDatabaseProductName());
            }
        } catch (SQLException e) {
            System.err.println("Database connection failed: " + e.getMessage());
        }
    }
}