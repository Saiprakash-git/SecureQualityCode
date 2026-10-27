import java.sql.*;

public class CustomerRetriever {

    private static final String DB_URL = "jdbc:mysql://localhost:3306/customerdb";
    private static final String DB_USER = "root";
    private static final String DB_PASSWORD = "password";

    public static class Customer {
        private String customerUsername;
        private String customerName;
        private String customerEmail;
        private String customerPhone;
        private String customerAddress;

        public Customer(String customerUsername, String customerName, String customerEmail,
                       String customerPhone, String customerAddress) {
            this.customerUsername = customerUsername;
            this.customerName = customerName;
            this.customerEmail = customerEmail;
            this.customerPhone = customerPhone;
            this.customerAddress = customerAddress;
        }

        public String getCustomerUsername() { return customerUsername; }
        public String getCustomerName() { return customerName; }
        public String getCustomerEmail() { return customerEmail; }
        public String getCustomerPhone() { return customerPhone; }
        public String getCustomerAddress() { return customerAddress; }

        @Override
        public String toString() {
            return "Customer{" +
                    "customerUsername='" + customerUsername + '\'' +
                    ", customerName='" + customerName + '\'' +
                    ", customerEmail='" + customerEmail + '\'' +
                    ", customerPhone='" + customerPhone + '\'' +
                    ", customerAddress='" + customerAddress + '\'' +
                    '}';
        }
    }

    public static Connection getConnection() throws SQLException {
        return DriverManager.getConnection(DB_URL, DB_USER, DB_PASSWORD);
    }

    public static Customer getCustomerByUsername(String customerUsername) {
        String sql = "SELECT customerusername, customername, customeremail, customerphone, customeraddress FROM customer WHERE customerusername = ?";
        Customer customer = null;

        try (Connection conn = getConnection();
             PreparedStatement pstmt = conn.prepareStatement(sql)) {

            pstmt.setString(1, customerUsername);

            try (ResultSet rs = pstmt.executeQuery()) {
                if (rs.next()) {
                    customer = new Customer(
                        rs.getString("customerusername"),
                        rs.getString("customername"),
                        rs.getString("customeremail"),
                        rs.getString("customerphone"),
                        rs.getString("customeraddress")
                    );
                }
            }
        } catch (SQLException e) {
            e.printStackTrace();
        }

        return customer;
    }

    public static void main(String[] args) {
        if (args.length < 1) {
            System.err.println("Usage: java CustomerRetriever <customerusername>");
            System.exit(1);
        }

        String customerUsername = args[0];
        Customer customer = getCustomerByUsername(customerUsername);

        if (customer != null) {
            System.out.println(customer);
        } else {
            System.out.println("Customer not found: " + customerUsername);
        }
    }
}