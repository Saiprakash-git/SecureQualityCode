import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;
import java.security.spec.InvalidKeySpecException;
import java.security.spec.KeySpec;
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
    private static final Pattern EMAIL_PATTERN = Pattern.compile("^[A-Za-z0-9+_.-]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,}$");
    private static final int SALT_LENGTH = 16;
    private static final int HASH_ITERATIONS = 65536;
    private static final int KEY_LENGTH = 256;

    private final String dbUrl;

    public static class User {
        private final int id;
        private final String name;
        private final String email;

        public User(int id, String name, String email) {
            this.id = id;
            this.name = name;
            this.email = email;
        }

        public int getId() {
            return id;
        }

        public String getName() {
            return name;
        }

        public String getEmail() {
            return email;
        }

        @Override
        public String toString() {
            return "User{id=" + id + ", name='" + name + "', email='" + email + "'}";
        }
    }

    public static class RegistrationResult {
        private final boolean success;
        private final String message;
        private final User user;

        public RegistrationResult(boolean success, String message, User user) {
            this.success = success;
            this.message = message;
            this.user = user;
        }

        public boolean isSuccess() {
            return success;
        }

        public String getMessage() {
            return message;
        }

        public User getUser() {
            return user;
        }
    }

    public UserRegistrationSystem(String dbUrl) {
        this.dbUrl = dbUrl;
        initializeDatabase();
    }

    private void initializeDatabase() {
        String createTableSQL = "CREATE TABLE IF NOT EXISTS users ("
                + "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                + "name TEXT NOT NULL, "
                + "email TEXT NOT NULL UNIQUE, "
                + "password_hash TEXT NOT NULL, "
                + "salt TEXT NOT NULL, "
                + "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
                + ");";

        try (Connection conn = getConnection();
             Statement stmt = conn.createStatement()) {
            stmt.execute(createTableSQL);
        } catch (SQLException e) {
            throw new RuntimeException("Failed to initialize database: " + e.getMessage(), e);
        }
    }

    protected Connection getConnection() throws SQLException {
        return DriverManager.getConnection(this.dbUrl);
    }

    public RegistrationResult register(String name, String email, String password) {
        String validationError = validateInputs(name, email, password);
        if (validationError != null) {
            return new RegistrationResult(false, validationError, null);
        }

        String normalizedEmail = email.trim().toLowerCase();
        String trimmedName = name.trim();

        if (isEmailTaken(normalizedEmail)) {
            return new RegistrationResult(false, "Email is already registered.", null);
        }

        byte[] salt = generateSalt();
        String passwordHash = hashPassword(password, salt);
        String saltBase64 = Base64.getEncoder().encodeToString(salt);

        String insertSQL = "INSERT INTO users(name, email, password_hash, salt) VALUES(?, ?, ?, ?)";

        try (Connection conn = getConnection();
             PreparedStatement pstmt = conn.prepareStatement(insertSQL, Statement.RETURN_GENERATED_KEYS)) {

            pstmt.setString(1, trimmedName);
            pstmt.setString(2, normalizedEmail);
            pstmt.setString(3, passwordHash);
            pstmt.setString(4, saltBase64);

            int affectedRows = pstmt.executeUpdate();
            if (affectedRows == 0) {
                return new RegistrationResult(false, "User registration failed, no rows affected.", null);
            }

            try (ResultSet generatedKeys = pstmt.getGeneratedKeys()) {
                if (generatedKeys.next()) {
                    int userId = generatedKeys.getInt(1);
                    User registeredUser = new User(userId, trimmedName, normalizedEmail);
                    return new RegistrationResult(true, "Registration successful.", registeredUser);
                } else {
                    return new RegistrationResult(false, "User registration failed, no ID obtained.", null);
                }
            }

        } catch (SQLException e) {
            return new RegistrationResult(false, "Database error during registration: " + e.getMessage(), null);
        }
    }

    public boolean isEmailTaken(String email) {
        String querySQL = "SELECT id FROM users WHERE email = ?";
        try (Connection conn = getConnection();
             PreparedStatement pstmt = conn.prepareStatement(querySQL)) {

            pstmt.setString(1, email);
            try (ResultSet rs = pstmt.executeQuery()) {
                return rs.next();
            }
        } catch (SQLException e) {
            throw new RuntimeException("Database error during email check: " + e.getMessage(), e);
        }
    }

    public User getUserByEmail(String email) {
        String querySQL = "SELECT id, name, email FROM users WHERE email = ?";
        try (Connection conn = getConnection();
             PreparedStatement pstmt = conn.prepareStatement(querySQL)) {

            pstmt.setString(1, email.trim().toLowerCase());
            try (ResultSet rs = pstmt.executeQuery()) {
                if (rs.next()) {
                    return new User(rs.getInt("id"), rs.getString("name"), rs.getString("email"));
                }
            }
        } catch (SQLException e) {
            throw new RuntimeException("Database error fetching user: " + e.getMessage(), e);
        }
        return null;
    }

    private String validateInputs(String name, String email, String password) {
        if (name == null || name.trim().isEmpty()) {
            return "Name cannot be empty.";
        }
        if (email == null || !EMAIL_PATTERN.matcher(email.trim()).matches()) {
            return "Invalid email address format.";
        }
        if (password == null || password.length() < 8) {
            return "Password must be at least 8 characters long.";
        }
        return null;
    }

    private byte[] generateSalt() {
        SecureRandom random = new SecureRandom();
        byte[] salt = new byte[SALT_LENGTH];
        random.nextBytes(salt);
        return salt;
    }

    private String hashPassword(String password, byte[] salt) {
        try {
            KeySpec spec = new PBEKeySpec(password.toCharArray(), salt, HASH_ITERATIONS, KEY_LENGTH);
            SecretKeyFactory factory = SecretKeyFactory.getInstance("PBKDF2WithHmacSHA256");
            byte[] hash = factory.generateSecret(spec).getEncoded();
            return Base64.getEncoder().encodeToString(hash);
        } catch (NoSuchAlgorithmException | InvalidKeySpecException e) {
            throw new RuntimeException("Error hashing password: " + e.getMessage(), e);
        }
    }

    public static void main(String[] args) {
        UserRegistrationSystem system = new UserRegistrationSystem(DEFAULT_DB_URL);
        Scanner scanner = new Scanner(System.in);

        System.out.println("=== User Registration System ===");

        System.out.print("Enter your name: ");
        String name = scanner.nextLine();

        System.out.print("Enter your email: ");
        String email = scanner.nextLine();

        System.out.print("Enter your password: ");
        String password = scanner.nextLine();

        RegistrationResult result = system.register(name, email, password);

        if (result.isSuccess()) {
            System.out.println("Success: " + result.getMessage());
            System.out.println("Registered User Details: " + result.getUser());
        } else {
            System.out.println("Error: " + result.getMessage());
        }

        scanner.close();
    }
}