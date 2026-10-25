import java.io.Console;
import java.security.GeneralSecurityException;
import java.security.SecureRandom;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.PreparedStatement;
import java.sql.SQLException;
import java.sql.Statement;
import java.util.Arrays;
import java.util.Locale;
import java.util.Scanner;
import java.util.regex.Pattern;
import javax.crypto.SecretKeyFactory;
import javax.crypto.spec.PBEKeySpec;

public class UserRegistrationSystem {
    private static final int SALT_LENGTH = 16;
    private static final int HASH_LENGTH_BITS = 256;
    private static final int ITERATIONS = 210_000;
    private static final SecureRandom SECURE_RANDOM = new SecureRandom();
    private static final Pattern EMAIL_PATTERN =
            Pattern.compile("^[^\\s@]+@[^\\s@]+\\.[^\\s@]+$");

    public static void main(String[] args) {
        String databaseUrl = System.getenv().getOrDefault("DB_URL", "jdbc:sqlite:users.db");

        try (Connection connection = DriverManager.getConnection(databaseUrl)) {
            initializeDatabase(connection);

            Console console = System.console();
            try (Scanner scanner = console == null ? new Scanner(System.in) : null) {
                String name = readLine(console, scanner, "Name: ");
                String email = readLine(console, scanner, "Email: ");
                char[] password = readPassword(console, scanner);

                try {
                    boolean registered = register(connection, name, email, password);
                    System.out.println(registered
                            ? "Registration successful."
                            : "An account with that email already exists.");
                } finally {
                    Arrays.fill(password, '\0');
                }
            }
        } catch (Exception e) {
            System.err.println("Registration failed: " + e.getMessage());
        }
    }

    private static void initializeDatabase(Connection connection) throws SQLException {
        String sql = "CREATE TABLE IF NOT EXISTS users ("
                + "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                + "name TEXT NOT NULL, "
                + "email TEXT NOT NULL COLLATE NOCASE UNIQUE, "
                + "password_salt BLOB NOT NULL, "
                + "password_hash BLOB NOT NULL, "
                + "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
                + ")";

        try (Statement statement = connection.createStatement()) {
            statement.executeUpdate(sql);
        }
    }

    private static boolean register(
            Connection connection, String name, String email, char[] password)
            throws SQLException, GeneralSecurityException {
        String normalizedName = name == null ? "" : name.trim();
        String normalizedEmail = email == null ? "" : email.trim().toLowerCase(Locale.ROOT);

        if (normalizedName.isEmpty()) {
            throw new IllegalArgumentException("Name is required.");
        }
        if (!EMAIL_PATTERN.matcher(normalizedEmail).matches()) {
            throw new IllegalArgumentException("A valid email address is required.");
        }
        if (password == null || password.length < 8) {
            throw new IllegalArgumentException("Password must contain at least 8 characters.");
        }

        byte[] salt = new byte[SALT_LENGTH];
        SECURE_RANDOM.nextBytes(salt);
        byte[] passwordHash = hashPassword(password, salt);

        String sql = "INSERT INTO users (name, email, password_salt, password_hash) "
                + "VALUES (?, ?, ?, ?) ON CONFLICT(email) DO NOTHING";

        try (PreparedStatement statement = connection.prepareStatement(sql)) {
            statement.setString(1, normalizedName);
            statement.setString(2, normalizedEmail);
            statement.setBytes(3, salt);
            statement.setBytes(4, passwordHash);
            return statement.executeUpdate() == 1;
        } finally {
            Arrays.fill(passwordHash, (byte) 0);
            Arrays.fill(salt, (byte) 0);
        }
    }

    private static byte[] hashPassword(char[] password, byte[] salt)
            throws GeneralSecurityException {
        PBEKeySpec spec = new PBEKeySpec(password, salt, ITERATIONS, HASH_LENGTH_BITS);
        try {
            return SecretKeyFactory.getInstance("PBKDF2WithHmacSHA256")
                    .generateSecret(spec)
                    .getEncoded();
        } finally {
            spec.clearPassword();
        }
    }

    private static String readLine(Console console, Scanner scanner, String prompt) {
        if (console != null) {
            return console.readLine("%s", prompt);
        }
        System.out.print(prompt);
        return scanner.nextLine();
    }

    private static char[] readPassword(Console console, Scanner scanner) {
        if (console != null) {
            char[] password = console.readPassword("Password: ");
            return password == null ? new char[0] : password;
        }
        System.out.print("Password: ");
        return scanner.nextLine().toCharArray();
    }
}