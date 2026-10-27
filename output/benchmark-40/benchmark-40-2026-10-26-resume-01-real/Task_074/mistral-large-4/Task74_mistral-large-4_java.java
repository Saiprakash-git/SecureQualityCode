import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.SQLException;

public class PostgresConnectionFactory {

    public static Connection getConnection(String url, String username, String password) throws SQLException {
        return DriverManager.getConnection(url, username, password);
    }

    public static Connection getConnection(String host, int port, String databaseName, String username, String password) throws SQLException {
        String url = String.format("jdbc:postgresql://%s:%d/%s", host, port, databaseName);
        return DriverManager.getConnection(url, username, password);
    }

    public static void main(String[] args) {
        try {
            Connection conn = getConnection("localhost", 5432, "mydatabase", "user", "password");
            System.out.println("Connection created: " + !conn.isClosed());
            conn.close();
        } catch (SQLException e) {
            e.printStackTrace();
        }
    }
}