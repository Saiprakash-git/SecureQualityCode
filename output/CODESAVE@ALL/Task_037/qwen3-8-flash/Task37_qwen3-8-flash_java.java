import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;

public class Main {
    public static String getUserInfo(String jdbcUrl, String databaseUser, String databasePassword, String username) {
        String sql = "SELECT id, username, email FROM users WHERE username = ?";

        try (Connection connection = DriverManager.getConnection(jdbcUrl, databaseUser, databasePassword);
             PreparedStatement statement = connection.prepareStatement(sql)) {

            statement.setString(1, username);

            try (ResultSet resultSet = statement.executeQuery()) {
                if (!resultSet.next()) {
                    return "No user found for username: " + username;
                }

                return "ID: " + resultSet.getInt("id") + "\n" +
                       "Username: " + resultSet.getString("username") + "\n" +
                       "Email: " + resultSet.getString("email");
            }

        } catch (SQLException e) {
            return "Database error: " + e.getMessage();
        }
    }

    public static void main(String[] args) {
        if (args.length < 4) {
            System.out.println("Usage: java Main <jdbcUrl> <databaseUser> <databasePassword> <username>");
            return;
        }

        String result = getUserInfo(args[0], args[1], args[2], args[3]);
        System.out.println(result);
    }
}