import java.io.*;
import java.nio.file.*;
import java.util.*;
import java.util.regex.*;

public class SecureInputProcessor {

    public static void main(String[] args) {
        Scanner scanner = new Scanner(System.in);

        try {
            System.out.print("Enter username: ");
            String username = scanner.nextLine();
            String validatedUsername = validateUsername(username);

            System.out.print("Enter age: ");
            String ageInput = scanner.nextLine();
            int validatedAge = validateAge(ageInput);

            System.out.print("Enter file path to read: ");
            String filePath = scanner.nextLine();
            String validatedPath = validateFilePath(filePath);

            int birthYear = calculateBirthYear(validatedAge);
            String fileContent = readFileSecurely(validatedPath);

            System.out.println("Username: " + sanitizeForOutput(validatedUsername));
            System.out.println("Birth year: " + birthYear);
            System.out.println("File content (first 100 chars): " +
                sanitizeForOutput(fileContent.substring(0, Math.min(100, fileContent.length()))));

        } catch (SecurityException e) {
            System.err.println("Security error: " + e.getMessage());
        } catch (Exception e) {
            System.err.println("Error: " + e.getMessage());
        } finally {
            scanner.close();
        }
    }

    public static String validateUsername(String input) {
        if (input == null || input.trim().isEmpty()) {
            throw new SecurityException("Username cannot be empty");
        }

        String trimmed = input.trim();

        if (trimmed.length() > 50) {
            throw new SecurityException("Username too long (max 50 characters)");
        }

        Pattern pattern = Pattern.compile("^[a-zA-Z0-9_-]+$");
        if (!pattern.matcher(trimmed).matches()) {
            throw new SecurityException("Username contains invalid characters");
        }

        return trimmed;
    }

    public static int validateAge(String input) {
        if (input == null || input.trim().isEmpty()) {
            throw new SecurityException("Age cannot be empty");
        }

        try {
            int age = Integer.parseInt(input.trim());

            if (age < 0 || age > 150) {
                throw new SecurityException("Age must be between 0 and 150");
            }

            return age;
        } catch (NumberFormatException e) {
            throw new SecurityException("Age must be a valid number");
        }
    }

    public static String validateFilePath(String input) {
        if (input == null || input.trim().isEmpty()) {
            throw new SecurityException("File path cannot be empty");
        }

        try {
            Path path = Paths.get(input.trim()).toAbsolutePath().normalize();

            Path allowedBase = Paths.get(System.getProperty("user.home")).toAbsolutePath().normalize();
            if (!path.startsWith(allowedBase)) {
                throw new SecurityException("Access denied: Path outside allowed directory");
            }

            if (!Files.exists(path)) {
                throw new SecurityException("File does not exist");
            }

            if (!Files.isReadable(path)) {
                throw new SecurityException("File is not readable");
            }

            String fileName = path.getFileName().toString().toLowerCase();
            if (fileName.startsWith(".") || fileName.contains("password") ||
                fileName.contains("secret") || fileName.contains("key")) {
                throw new SecurityException("Access to sensitive files denied");
            }

            return path.toString();
        } catch (InvalidPathException e) {
            throw new SecurityException("Invalid file path format");
        }
    }

    public static int calculateBirthYear(int age) {
        if (age < 0 || age > 150) {
            throw new IllegalArgumentException("Invalid age");
        }
        return Calendar.getInstance().get(Calendar.YEAR) - age;
    }

    public static String readFileSecurely(String filePath) {
        try {
            Path path = Paths.get(filePath);

            if (!Files.isRegularFile(path)) {
                throw new SecurityException("Path is not a regular file");
            }

            long maxSize = 1024 * 1024;
            if (Files.size(path) > maxSize) {
                throw new SecurityException("File too large (max 1MB)");
            }

            byte[] bytes = Files.readAllBytes(path);
            return new String(bytes, "UTF-8");

        } catch (IOException e) {
            throw new SecurityException("Error reading file: " + e.getMessage());
        }
    }

    public static String sanitizeForOutput(String input) {
        if (input == null) return "";

        return input.replace("&", "&amp;")
                   .replace("<", "&lt;")
                   .replace(">", "&gt;")
                   .replace("\"", "&quot;")
                   .replace("'", "&#x27;");
    }
}