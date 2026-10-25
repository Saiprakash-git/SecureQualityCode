import java.nio.charset.StandardCharsets;
import java.security.SecureRandom;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;
import java.util.Base64;
import java.util.Scanner;
import java.util.regex.Pattern;
import javax.crypto.SecretKeyFactory;
import javax.crypto.spec.PBEKeySpec;

public class UserRegistrationSystem {

    private static final String DEFAULT_DB_URL = "jdbc:sqlite:users.db";
    private static final Pattern EMAIL_PATTERN =
            Pattern.compile("^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,}$");
    private static final int MIN_PASSWORD_LENGTH = 8;
    private static final int SALT_LENGTH = 16;
    private static final int HASH_ITERATIONS = 65536;
    private static final int KEY_LENGTH = 256;

    public static Connection connect(String url) throws SQLException {
        return DriverManager.getConnection(url);
    }

    public static void initializeDatabase(Connection conn) throws SQLException {
        String sql = "CREATE TABLE IF NOT EXISTS users ("
                + "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                + "name TEXT NOT NULL, "
                + "email TEXT NOT NULL UNIQUE, "
                + "password_hash TEXT NOT NULL, "
                + "salt TEXT NOT NULL, "
                + "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP)";
        try (Statement stmt = conn.createStatement()) {
            stmt.execute(sql);
        }
    }

    public static String validateName(String name) {
        if (name == null || name.trim().isEmpty()) {
            return "Name is required.";
        }
        if (name.trim().length() < 2) {
            return "Name must be at least 2 characters.";
        }
        if (name.trim().length() > 100) {
            return "Name must be at most 100 characters.";
        }
        return null;
    }

    public static String validateEmail(String email) {
        if (email == null || email.trim().isEmpty()) {
            return "Email is required.";
        }
        if (!EMAIL_PATTERN.matcher(email.trim()).matches()) {
            return "Email format is invalid.";
        }
        return null;
    }

    public static String validatePassword(String password) {
        if (password == null || password.length() < MIN_PASSWORD_LENGTH) {
            return "Password must be at least " + MIN_PASSWORD_LENGTH + " characters.";
        }
        boolean hasLetter = false;
        boolean hasDigit = false;
        for (char c : password.toCharArray()) {
            if (Character.isLetter(c)) hasLetter = true;
            if (Character.isDigit(c)) hasDigit = true;
        }
        if (!hasLetter || !hasDigit) {
            return "Password must contain at least one letter and one digit.";
        }
        return null;
    }

    public static String generateSalt() {
        byte[] salt = new byte[SALT_LENGTH];
        new SecureRandom().nextBytes(salt);
        return Base64.getEncoder().encodeToString(salt);
    }

    public static String hashPassword(String password, String saltBase64) throws Exception {
        byte[] salt = Base64.getDecoder().decode(saltBase64);
        PBEKeySpec spec = new PBEKeySpec(password.toCharArray(), salt, HASH_ITERATIONS, KEY_LENGTH);
        SecretKeyFactory factory = SecretKeyFactory.getInstance("PBKDF2WithHmacSHA256");
        byte[] hash = factory.generateSecret(spec).getEncoded();
        spec.clearPassword();
        return Base64.getEncoder().encodeToString(hash);
    }

    public static boolean emailExists(Connection conn, String email) throws SQLException {
        String sql = "SELECT 1 FROM users WHERE email = ?";
        try (PreparedStatement ps = conn.prepareStatement(sql)) {
            ps.setString(1, email.trim().toLowerCase());
            try (ResultSet rs = ps.executeQuery()) {
                return rs.next();
            }
        }
    }

    public static String registerUser(Connection conn, String name, String email, String password) {
        String error = validateName(name);
        if (error != null) return "ERROR: " + error;
        error = validateEmail(email);
        if (error != null) return "ERROR: " + error;
        error = validatePassword(password);
        if (error != null) return "ERROR: " + error;

        String normalizedEmail = email.trim().toLowerCase();
        try {
            if (emailExists(conn, normalizedEmail)) {
                return "ERROR: An account with this email already exists.";
            }
            String salt = generateSalt();
            String hash = hashPassword(password, salt);
            String sql = "INSERT INTO users (name, email, password_hash, salt) VALUES (?, ?, ?, ?)";
            try (PreparedStatement ps = conn.prepareStatement(sql)) {
                ps.setString(1, name.trim());
                ps.setString(2, normalizedEmail);
                ps.setString(3, hash);
                ps.setString(4, salt);
                ps.executeUpdate();
            }
            return "SUCCESS: User registered.";
        } catch (SQLException e) {
            if (e.getMessage() != null && e.getMessage().toUpperCase().contains("UNIQUE")) {
                return "ERROR: An account with this email already exists.";
            }
            return "ERROR: Database error: " + e.getMessage();
        } catch (Exception e) {
            return "ERROR: Password processing failed: " + e.getMessage();
        }
    }

    public static boolean verifyLogin(Connection conn, String email, String password) {
        String sql = "SELECT password_hash, salt FROM users WHERE email = ?";
        try (PreparedStatement ps = conn.prepareStatement(sql)) {
            ps.setString(1, email == null ? "" : email.trim().toLowerCase());
            try (ResultSet rs = ps.executeQuery()) {
                if (!rs.next()) return false;
                String storedHash = rs.getString("password_hash");
                String salt = rs.getString("salt");
                return storedHash.equals(hashPassword(password, salt));
            }
        } catch (Exception e) {
            return false;
        }
    }

    public static void main(String[] args) {
        String dbUrl = args.length > 0 ? args[0] : DEFAULT_DB_URL;
        Scanner scanner = new Scanner(System.in, StandardCharsets.UTF_8.name());

        try (Connection conn = connect(dbUrl)) {
            initializeDatabase(conn);
            System.out.println("User Registration System (database: " + dbUrl + ")");

            while (true) {
                System.out.println();
                System.out.println("1) Register");
                System.out.println("2) Verify login");
                System.out.println("3) Exit");
                System.out.print("Choose an option: ");
                String choice = scanner.hasNextLine() ? scanner.nextLine().trim() : "3";

                if (choice.equals("1")) {
                    System.out.print("Name: ");
                    String name = scanner.nextLine();
                    System.out.print("Email: ");
                    String email = scanner.nextLine();
                    System.out.print("Password: ");
                    String password = scanner.nextLine();
                    System.out.println(registerUser(conn, name, email, password));
                } else if (choice.equals("2")) {
                    System.out.print("Email: ");
                    String email = scanner.nextLine();
                    System.out.print("Password: ");
                    String password = scanner.nextLine();
                    System.out.println(verifyLogin(conn, email, password)
                            ? "SUCCESS: Credentials are valid."
                            : "ERROR: Invalid email or password.");
                } else if (choice.equals("3")) {
                    System.out.println("Goodbye.");
                    break;
                } else {
                    System.out.println("Invalid option.");
                }
            }
        } catch (SQLException e) {
            System.err.println("Database connection failed: " + e.getMessage());
        } finally {
            scanner.close();
        }
    }
}