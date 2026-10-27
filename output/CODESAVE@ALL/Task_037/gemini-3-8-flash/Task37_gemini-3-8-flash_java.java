import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.util.Optional;
import java.util.Scanner;

public class UserDatabaseApp {

    public static class User {
        private final int id;
        private final String username;
        private final String email;
        private final String fullName;

        public User(int id, String username, String email, String fullName) {
            this.id = id;
            this.username = username;
            this.email = email;
            this.fullName = fullName;
        }

        public int getId() {
            return id;
        }

        public String getUsername() {
            return username;
        }

        public String getEmail() {
            return email;
        }

        public String getFullName() {
            return fullName;
        }

        @Override
        public String toString() {
            return "User{" +
                    "id=" + id +
                    ", username='" + username + '\'' +
                    ", email='" + email + '\'' +
                    ", fullName='" + fullName + '\'' +
                    '}';
        }
    }

    public static Connection createConnection(String url, String user, String password) throws SQLException {
        return DriverManager.getConnection(url, user, password);
    }

    public static Optional<User> getUserByUsername(Connection connection, String username) throws SQLException {
        String sql = "SELECT id, username, email, full_name FROM users WHERE username = ?";

        try (PreparedStatement statement = connection.prepareStatement(sql)) {
            statement.setString(1, username);

            try (ResultSet resultSet = statement.executeQuery()) {
                if (resultSet.next()) {
                    User user = new User(
                            resultSet.getInt("id"),
                            resultSet.getString("username"),
                            resultSet.getString("email"),
                            resultSet.getString("full_name")
                    );
                    return Optional.of(user);
                }
            }
        }

        return Optional.empty();
    }

    public static void main(String[] args) {
        String dbUrl = System.getenv().getOrDefault("DB_URL", "jdbc:mysql://localhost:3306/app_db");
        String dbUser = System.getenv().getOrDefault("DB_USER", "root");
        String dbPassword = System.getenv().getOrDefault("DB_PASSWORD", "password");

        Scanner scanner = new Scanner(System.in);
        System.out.print("Enter username to search: ");
        String username = scanner.nextLine().trim();

        try (Connection connection = createConnection(dbUrl, dbUser, dbPassword)) {
            Optional<User> userOptional = getUserByUsername(connection, username);

            if (userOptional.isPresent()) {
                System.out.println("User found: " + userOptional.get());
            } else {
                System.out.println("No user found with username: " + username);
            }
        } catch (SQLException e) {
            System.err.println("Database error occurred: " + e.getMessage());
        } finally {
            scanner.close();
        }
    }
}