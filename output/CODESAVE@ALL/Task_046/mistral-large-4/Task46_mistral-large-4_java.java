import java.sql.*;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;
import java.util.Base64;

public class UserRegistrationSystem {
    private static final String DB_URL = "jdbc:h2:./userdb;AUTO_SERVER=TRUE";
    private static final String DB_USER = "sa";
    private static final String DB_PASSWORD = "";
    private Connection connection;

    public UserRegistrationSystem() throws SQLException {
        this.connection = DriverManager.getConnection(DB_URL, DB_USER, DB_PASSWORD);
        initializeDatabase();
    }

    private void initializeDatabase() throws SQLException {
        String createTableSQL = "CREATE TABLE IF NOT EXISTS users (" +
                "id INT AUTO_INCREMENT PRIMARY KEY, " +
                "name VARCHAR(100) NOT NULL, " +
                "email VARCHAR(100) NOT NULL UNIQUE, " +
                "password_hash VARCHAR(255) NOT NULL, " +
                "salt VARCHAR(255) NOT NULL, " +
                "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP" +
                ")";
        try (Statement stmt = connection.createStatement()) {
            stmt.execute(createTableSQL);
        }
    }

    public boolean register