import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.util.Optional;

public class CustomerLookup {

    public static class Customer {
        private final int customerId;
        private final String customerUsername;
        private final String firstName;
        private final String lastName;
        private final String email;
        private final String phone;
        private final String address;

        public Customer(int customerId, String customerUsername, String firstName,
                        String lastName, String email, String phone, String address) {
            this.customerId = customerId;
            this.customerUsername = customerUsername;
            this.firstName = firstName;
            this.lastName = lastName;
            this.email = email;
            this.phone = phone;
            this.address = address;
        }

        public int getCustomerId() { return customerId; }
        public String getCustomerUsername() { return customerUsername; }
        public String getFirstName() { return firstName; }
        public String getLastName() { return lastName; }
        public String getEmail() { return email; }
        public String getPhone() { return phone; }
        public String getAddress() { return address; }

        @Override
        public String toString() {
            return "Customer{" +
                    "customerId=" + customerId +
                    ", customerUsername='" + customerUsername + '\'' +
                    ", firstName='" + firstName + '\'' +
                    ", lastName='" + lastName + '\'' +
                    ", email='" + email + '\'' +
                    ", phone='" + phone + '\'' +
                    ", address='" + address + '\'' +
                    '}';
        }
    }

    public static Optional<Customer> fetchCustomerByUsername(Connection connection, String customerUsername) throws SQLException {
        // Assumed columns in the "customer" table:
        // customer_id, customerusername, first_name, last_name, email, phone, address
        final String sql = "SELECT customer_id, customerusername, first_name, last_name, email, phone, address " +
                           "FROM customer WHERE customerusername = ?";

        try (PreparedStatement ps = connection.prepareStatement(sql)) {
            ps.setString(1, customerUsername);
            try (ResultSet rs = ps.executeQuery()) {
                if (rs.next()) {
                    Customer customer = new Customer(
                            rs.getInt("customer_id"),
                            rs.getString("customerusername"),
                            rs.getString("first_name"),
                            rs.getString("last_name"),
                            rs.getString("email"),
                            rs.getString("phone"),
                            rs.getString("address")
                    );
                    return Optional.of(customer);
                }
            }
        }
        return Optional.empty();
    }

    public static void main(String[] args) {
        if (args.length < 1 || args[0] == null || args[0].trim().isEmpty()) {
            System.err.println("Usage: java CustomerLookup <customerusername>");
            System.exit(1);
        }

        String customerUsername = args[0].trim();

        // Connection settings are read from environment variables, with defaults.
        String url = System.getenv().getOrDefault("DB_URL", "jdbc:mysql://localhost:3306/shop");
        String user = System.getenv().getOrDefault("DB_USER", "root");
        String password = System.getenv().getOrDefault("DB_PASSWORD", "");

        try (Connection connection = DriverManager.getConnection(url, user, password)) {
            Optional<Customer> result = fetchCustomerByUsername(connection, customerUsername);
            if (result.isPresent()) {
                System.out.println(result.get());
            } else {
                System.out.println("No customer found with username: " + customerUsername);
            }
        } catch (SQLException e) {
            System.err.println("Database error: " + e.getMessage());
            System.exit(2);
        }
    }
}