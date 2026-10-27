import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Timestamp;

public class CustomerLookup {

    public record Customer(
            int id,
            String username,
            String email,
            String firstName,
            String lastName,
            Timestamp createdAt
    ) {}

    public static Customer getCustomerByUsername(Connection connection, String username) throws SQLException {
        String sql = "SELECT id, customerusername, email, first_name, last_name, created_at " +
                     "FROM customer WHERE customerusername = ?";

        try (PreparedStatement statement = connection.prepareStatement(sql)) {
            statement.setString(1, username);
            try (ResultSet resultSet = statement.executeQuery()) {
                if (resultSet.next()) {
                    return new Customer(
                            resultSet.getInt("id"),
                            resultSet.getString("customerusername"),
                            resultSet.getString("email"),
                            resultSet.getString("first_name"),
                            resultSet.getString("last_name"),
                            resultSet.getTimestamp("created_at")
                    );
                }
            }
        }
        return null;
    }

    public static Connection createConnection() throws SQLException {
        String url = System.getenv().getOrDefault("DB_URL", "jdbc:postgresql://localhost:5432/customer_db");
        String user = System.getenv().getOrDefault("DB_USER", "postgres");
        String password = System.getenv().getOrDefault("DB_PASSWORD", "password");
        return DriverManager.getConnection(url, user, password);
    }

    public static void main(String[] args) {
        if (args.length < 1) {
            System.err.println("Usage: java CustomerLookup <customerusername>");
            System.exit(1);
        }

        String username = args[0];

        try (Connection connection = createConnection()) {
            Customer customer = getCustomerByUsername(connection, username);
            if (customer != null) {
                System.out.println("Customer Found:");
                System.out.println("ID: " + customer.id());
                System.out.println("Username: " + customer.username());
                System.out.println("Email: " + customer.email());
                System.out.println("First Name: " + customer.firstName());
                System.out.println("Last Name: " + customer.lastName());
                System.out.println("Created At: " + customer.createdAt());
            } else {
                System.out.println("No customer found with username: " + username);
            }
        } catch (SQLException e) {
            System.err.println("Database error occurred: " + e.getMessage());
            e.printStackTrace();
            System.exit(2);
        }
    }
}