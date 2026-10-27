import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.ResultSetMetaData;
import java.sql.SQLException;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.Map;

public class Main {

    public static final class Customer {
        private final String customerUsername;
        private final Map<String, Object> attributes;

        public Customer(String customerUsername, Map<String, Object> attributes) {
            this.customerUsername = customerUsername;
            this.attributes = Collections.unmodifiableMap(new LinkedHashMap<String, Object>(attributes));
        }

        public String getCustomerUsername() {
            return customerUsername;
        }

        public Map<String, Object> getAttributes() {
            return attributes;
        }

        @Override
        public String toString() {
            return "Customer{customerUsername='" + customerUsername + "', attributes=" + attributes + "}";
        }
    }

    public static Customer retrieveCustomer(String customerUsername, String dbUrl, String dbUser, String dbPassword)
            throws SQLException {
        if (customerUsername == null || customerUsername.trim().isEmpty()) {
            return null;
        }

        if (dbUrl == null || dbUrl.trim().isEmpty()) {
            throw new SQLException("Database URL is required.");
        }

        if (dbUser == null) {
            dbUser = "";
        }

        if (dbPassword == null) {
            dbPassword = "";
        }

        String sql = "SELECT * FROM customer WHERE customerusername = ?";

        try (Connection connection = DriverManager.getConnection(dbUrl, dbUser, dbPassword);
             PreparedStatement statement = connection.prepareStatement(sql)) {

            statement.setString(1, customerUsername);

            try (ResultSet resultSet = statement.executeQuery()) {
                if (!resultSet.next()) {
                    return null;
                }

                ResultSetMetaData metaData = resultSet.getMetaData();
                Map<String, Object> attributes = new LinkedHashMap<String, Object>();

                for (int columnIndex = 1; columnIndex <= metaData.getColumnCount(); columnIndex++) {
                    String columnName = metaData.getColumnLabel(columnIndex);

                    if (columnName == null || columnName.isEmpty()) {
                        columnName = metaData.getColumnName(columnIndex);
                    }

                    attributes.put(columnName, resultSet.getObject(columnIndex));
                }

                return new Customer(customerUsername, attributes);
            }
        }
    }

    public static void main(String[] args) {
        if (args.length < 1) {
            System.err.println("Usage: java Main <customerUsername>");
            return;
        }

        String customerUsername = args[0];
        String dbUrl = System.getenv("DB_URL");
        String dbUser = System.getenv("DB_USER");
        String dbPassword = System.getenv("DB_PASSWORD");

        try {
            Customer customer = retrieveCustomer(customerUsername, dbUrl, dbUser, dbPassword);

            if (customer == null) {
                System.out.println("No customer found for username: " + customerUsername);
            } else {
                System.out.println(customer);
            }
        } catch (SQLException exception) {
            System.err.println("Database error: " + exception.getMessage());
        }
    }
}