import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.util.Scanner;

public class DatabaseUserQuery {

    public static void main(String[] args) {
        String dbUrl = "jdbc:mysql://localhost:3306/testdb";
        String dbUser = "root";
        String dbPassword = "password";

        Scanner scanner = new Scanner(System.in);
        System.out.print("Enter username to search: ");
        String searchUsername = scanner.nextLine();
        scanner.close();

        UserRecord user = fetchUserByUsername(dbUrl, dbUser, dbPassword, searchUsername);

        if (user != null) {
            System.out.println("ID: " + user.getId());
            System.out.println("Username: " + user.getUsername());
            System.out.println("Email: " + user.getEmail());
            System.out.println("Full Name: " + user.getFullName());
        } else {
            System.out.println("No user found with username: " + searchUsername);
        }
    }

    public static UserRecord fetchUserByUsername(String dbUrl, String dbUser, String dbPassword, String username) {
        String query = "SELECT id, username, email, full_name FROM users WHERE username = ?";

        try (Connection connection = DriverManager.getConnection(dbUrl, dbUser, dbPassword);
             PreparedStatement statement = connection.prepareStatement(query)) {

            statement.setString(1, username);

            try (ResultSet resultSet = statement.executeQuery()) {
                if (resultSet.next()) {
                    return new UserRecord(
                        resultSet.getInt("id"),
                        resultSet.getString("username"),
                        resultSet.getString("email"),
                        resultSet.getString("full_name")
                    );
                }
            }
        } catch (SQLException e) {
            System.err.println("Database error: " + e.getMessage());
        }

        return null;
    }

    public static class UserRecord {
        private final int id;
        private final String username;
        private final String email;
        private final String fullName;

        public UserRecord(int id, String username, String email, String fullName) {
            this.id = id;
            this.username = username;
            this.email = email;
            this.fullName = fullName;
        }

        public int getId() { return id; }
        public String getUsername() { return username; }
        public String getEmail() { return email; }
        public String getFullName() { return fullName; }
    }
}