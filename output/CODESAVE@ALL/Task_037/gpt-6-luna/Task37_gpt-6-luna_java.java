import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;

public class Main {
    public static void main(String[] args) throws Exception {
        String username = args.length > 0
                ? args[0]
                : new BufferedReader(new InputStreamReader(System.in)).readLine();

        if (username == null || username.trim().isEmpty()) {
            System.err.println("A username is required.");
            return;
        }

        String databaseUrl = requiredEnvironmentVariable("DB_URL");
        String databaseUser = requiredEnvironmentVariable("DB_USER");
        String databasePassword = requiredEnvironmentVariable("DB_PASSWORD");

        try (Connection connection = DriverManager.getConnection(
                databaseUrl, databaseUser, databasePassword)) {
            retrieveUser(connection, username.trim());
        }
    }

    private static String requiredEnvironmentVariable(String name) {
        String value = System.getenv(name);
        if (value == null || value.isEmpty()) {
            throw new IllegalStateException("Missing environment variable: " + name);
        }
        return value;
    }

    private static void retrieveUser(Connection connection, String username)
            throws SQLException {
        String sql = "SELECT id, username, email FROM users WHERE username = ?";

        try (PreparedStatement statement = connection.prepareStatement(sql)) {
            statement.setString(1, username);

            try (ResultSet results = statement.executeQuery()) {
                boolean found = false;
                while (results.next()) {
                    found = true;
                    System.out.printf(
                            "id=%d, username=%s, email=%s%n",
                            results.getLong("id"),
                            results.getString("username"),
                            results.getString("email"));
                }
                if (!found) {
                    System.out.println("No user found.");
                }
            }
        }
    }
}