import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.ResultSetMetaData;
import java.sql.SQLException;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.Scanner;

public class UserLookup {

    public static Connection connect(String jdbcUrl, String dbUser, String dbPassword) throws SQLException {
        return DriverManager.getConnection(jdbcUrl, dbUser, dbPassword);
    }

    public static Map<String, Object> findUserByUsername(Connection conn, String username) throws SQLException {
        if (conn == null) {
            throw new IllegalArgumentException("Connection must not be null");
        }
        if (username == null || username.trim().isEmpty()) {
            throw new IllegalArgumentException("Username must not be empty");
        }

        String sql = "SELECT id, username, email, full_name, created_at FROM users WHERE username = ?";

        try (PreparedStatement ps = conn.prepareStatement(sql)) {
            ps.setString(1, username.trim());
            try (ResultSet rs = ps.executeQuery()) {
                if (!rs.next()) {
                    return null;
                }
                Map<String, Object> user = new LinkedHashMap<>();
                ResultSetMetaData md = rs.getMetaData();
                for (int i = 1; i <= md.getColumnCount(); i++) {
                    user.put(md.getColumnLabel(i), rs.getObject(i));
                }
                return user;
            }
        }
    }

    public static String formatUser(Map<String, Object> user) {
        if (user == null) {
            return "User not found.";
        }
        StringBuilder sb = new StringBuilder("User information:\n");
        for (Map.Entry<String, Object> e : user.entrySet()) {
            sb.append(String.format("  %-12s: %s%n", e.getKey(), e.getValue()));
        }
        return sb.toString();
    }

    public static String lookup(String jdbcUrl, String dbUser, String dbPassword, String username) {
        try (Connection conn = connect(jdbcUrl, dbUser, dbPassword)) {
            Map<String, Object> user = findUserByUsername(conn, username);
            return formatUser(user);
        } catch (SQLException e) {
            return "Database error: " + e.getMessage();
        } catch (IllegalArgumentException e) {
            return "Input error: " + e.getMessage();
        }
    }

    public static void main(String[] args) {
        if (args.length < 3) {
            System.out.println("Usage: java UserLookup <jdbcUrl> <dbUser> <dbPassword> [username]");
            System.exit(1);
        }

        String jdbcUrl = args[0];
        String dbUser = args[1];
        String dbPassword = args[2];

        String username;
        if (args.length >= 4) {
            username = args[3];
        } else {
            try (Scanner scanner = new Scanner(System.in)) {
                System.out.print("Enter username: ");
                username = scanner.hasNextLine() ? scanner.nextLine() : "";
            }
        }

        System.out.println(lookup(jdbcUrl, dbUser, dbPassword, username));
    }
}