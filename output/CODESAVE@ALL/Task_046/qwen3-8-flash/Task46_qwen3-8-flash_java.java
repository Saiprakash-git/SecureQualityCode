import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;
import java.util.Base64;
import java.util.List;
import java.util.Locale;
import java.util.Scanner;
import java.util.regex.Pattern;

public class Main {
    private static final Pattern NAME_PATTERN = Pattern.compile("^[A-Za-z .'-]{2,}$");
    private static final Pattern EMAIL_PATTERN = Pattern.compile("^[A-Za-z0-9+_.-]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,}$");

    static class User {
        private final String name;
        private final String email;
        private final String passwordHash;

        User(String name, String email, String passwordHash) {
            this.name = name;
            this.email = email;
            this.passwordHash = passwordHash;
        }

        String getName() {
            return name;
        }

        String getEmail() {
            return email;
        }

        String getPasswordHash() {
            return passwordHash;
        }
    }

    static class UserDatabase {
        private final Path databaseFile;
        private final List<User> users;

        UserDatabase(String filePath) {
            this.databaseFile = Paths.get(filePath);
            this.users = new ArrayList<User>();
            loadFromFile(this.databaseFile);
        }

        private boolean loadFromFile(Path file) {
            if (!Files.exists(file)) {
                return true;
            }

            try {
                List<String> lines = Files.readAllLines(file, StandardCharsets.UTF_8);

                for (String line : lines) {
                    if (line == null || line.trim().isEmpty()) {
                        continue;
                    }

                    String[] parts = line.split("\\|", -1);
                    if (parts.length != 3) {
                        continue;
                    }

                    try {
                        String name = decode(parts[0]);
                        String email = decode(parts[1]);
                        String passwordHash = decode(parts[2]);
                        users.add(new User(name, email, passwordHash));
                    } catch (IllegalArgumentException ex) {
                        continue;
                    }
                }

                return true;
            } catch (IOException ex) {
                return false;
            }
        }

        private boolean saveToFile(Path file, List<User> usersToSave) {
            try {
                List<String> lines = new ArrayList<String>();

                for (User user : usersToSave) {
                    String line = encode(user.getName()) + "|" + encode(user.getEmail()) + "|" + encode(user.getPasswordHash());
                    lines.add(line);
                }

                Files.write(file, lines, StandardCharsets.UTF_8);
                return true;
            } catch (IOException ex) {
                return false;
            }
        }

        private String encode(String value) {
            return Base64.getEncoder().encodeToString(value.getBytes(StandardCharsets.UTF_8));
        }

        private String decode(String value) {
            return new String(Base64.getDecoder().decode(value), StandardCharsets.UTF_8);
        }

        boolean userExists(String email) {
            for (User user : users) {
                if (user.getEmail().equalsIgnoreCase(email)) {
                    return true;
                }
            }
            return false;
        }

        boolean addUser(User user) {
            if (user == null || userExists(user.getEmail())) {
                return false;
            }

            users.add(user);

            if (!saveToFile(databaseFile, users)) {
                users.remove(users.size() - 1);
                return false;
            }

            return true;
        }

        List<User> getUsers() {
            return new ArrayList<User>(users);
        }
    }

    private static String readInput(Scanner scanner, String prompt) {
        System.out.print(prompt);
        return scanner.nextLine();
    }

    private static String hashPassword(String password) throws NoSuchAlgorithmException {
        MessageDigest digest = MessageDigest.getInstance("SHA-256");
        byte[] hashBytes = digest.digest(password.getBytes(StandardCharsets.UTF_8));

        StringBuilder hexString = new StringBuilder();
        for (byte b : hashBytes) {
            String hex = Integer.toHexString(0xff & b);
            if (hex.length() == 1) {
                hexString.append('0');
            }
            hexString.append(hex);
        }

        return hexString.toString();
    }

    public static String registerUser(String name, String email, String password, UserDatabase database) throws NoSuchAlgorithmException {
        if (name == null || !NAME_PATTERN.matcher(name.trim()).matches()) {
            return "Invalid name. Name must contain at least 2 letters and may include spaces, apostrophes, hyphens, or periods.";
        }

        if (email == null || !EMAIL_PATTERN.matcher(email.trim()).matches()) {
            return "Invalid email address.";
        }

        if (password == null || password.length() < 8) {
            return "Invalid password. Password must be at least 8