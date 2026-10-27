import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.SQLException;
import java.util.Properties;

public class PostgresConnectionFactory {

    public static Connection createConnection(String host, int port, String database,
                                              String user, String password) throws SQLException {
        String url = "jdbc:postgresql://" + host + ":" + port + "/" + database;
        Properties props = new Properties();
        props.setProperty("user", user);
        props.setProperty("password", password);
        return DriverManager.getConnection(url, props);
    }

    public static Connection createConnection(String url, String user, String password) throws SQLException {
        Properties props = new Properties();
        props.setProperty("user", user);
        props.setProperty("password", password);
        return DriverManager.getConnection(url, props);
    }

    public static void main(String[] args) {
        String host = args.length > 0 ? args[0] : "localhost";
        int port = args.length > 1 ? Integer.parseInt(args[1]) : 5432;
        String database = args.length > 2 ? args[2] : "postgres";
        String user = args.length > 3 ? args[3] : "postgres";
        String password = args.length > 4 ? args[4] : "";

        try (Connection conn = createConnection(host, port, database, user, password)) {
            System.out.println("Connected: " + !conn.isClosed());
            System.out.println("Database product: " + conn.getMetaData().getDatabaseProductName());
        } catch (SQLException e) {
            System.err.println("Connection failed: " + e.getMessage());
        }
    }
}