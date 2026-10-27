import java.io.IOException;
import java.io.Reader;
import java.lang.reflect.Array;
import java.math.BigDecimal;
import java.sql.Array as SqlArray;
import java.sql.Blob;
import java.sql.Clob;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.ResultSetMetaData;
import java.sql.SQLException;
import java.sql.SQLXML;
import java.util.Base64;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.Properties;

public class CustomerLookup {
    public static final class Customer {
        private final LinkedHashMap<String, Object> fields;

        public Customer(LinkedHashMap<String, Object> fields) {
            this.fields = fields;
        }

        public LinkedHashMap<String, Object> getFields() {
            return fields;
        }
    }

    public static Customer findCustomer(Connection connection, String customerUsername)
            throws SQLException {
        String sql = "SELECT * FROM customer WHERE customerusername = ?";
        try (PreparedStatement statement = connection.prepareStatement(sql)) {
            statement.setString(1, customerUsername);
            try (ResultSet resultSet = statement.executeQuery()) {
                if (!resultSet.next()) {
                    return null;
                }

                ResultSetMetaData metadata = resultSet.getMetaData();
                LinkedHashMap<String, Object> fields = new LinkedHashMap<>();
                for (int column = 1; column <= metadata.getColumnCount(); column++) {
                    String name = metadata.getColumnLabel(column);
                    fields.put(name, toJsonValue(resultSet.getObject(column)));
                }
                return new Customer(fields);
            }
        }
    }

    private static Object toJsonValue(Object value) throws SQLException {
        if (value == null || value instanceof String || value instanceof Boolean
                || value instanceof Integer || value instanceof Long
                || value instanceof Short || value instanceof Byte
                || value instanceof BigDecimal || value instanceof Double
                || value instanceof Float) {
            return value;
        }
        if (value instanceof byte[]) {
            return Base64.getEncoder().encodeToString((byte[]) value);
        }
        if (value instanceof Blob) {
            Blob blob = (Blob) value;
            try {
                return Base64.getEncoder().encodeToString(blob.getBytes(1, (int) blob.length()));
            } finally {
                blob.free();
            }
        }
        if (value instanceof Clob) {
            Clob clob = (Clob) value;
            try {
                StringBuilder text = new StringBuilder();
                try (Reader reader = clob.getCharacterStream()) {
                    char[] buffer = new char[4096];
                    int count;
                    while ((count = reader.read(buffer)) != -1) {
                        text.append(buffer, 0, count);
                    }
                } catch (IOException exception) {
                    throw new SQLException("Unable to read customer data", exception);
                }
                return text.toString();
            } finally {
                clob.free();
            }
        }
        if (value instanceof SQLXML) {
            SQLXML xml = (SQLXML) value;
            try {
                return xml.getString();
            } finally {
                xml.free();
            }
        }
        if (value instanceof java.sql.Array) {
            java.sql.Array sqlArray = (java.sql.Array) value;
            try {
                Object array = sqlArray.getArray();
                int length = Array.getLength(array);
                java.util.List<Object> items = new java.util.ArrayList<>(length);
                for (int index = 0; index < length; index++) {
                    items.add(toJsonValue(Array.get(array, index)));
                }
                return items;
            } finally {
                sqlArray.free();
            }
        }
        if (value instanceof Number) {
            return value;
        }
        return value.toString();
    }

    public static String toJson(Object value) {
        if (value == null) {
            return "null";
        }
        if (value instanceof Customer) {
            return toJson(((Customer) value).getFields());
        }
        if (value instanceof String || value instanceof Character) {
            return quote(value.toString());
        }
        if (value instanceof Boolean || value instanceof Number) {
            return value.toString();
        }
        if (value instanceof Map<?, ?>) {
            StringBuilder json = new StringBuilder("{");
            boolean first = true;
            for (Map.Entry<?, ?> entry : ((Map<?, ?>) value).entrySet()) {
                if (!first) {
                    json.append(',');
                }
                first = false;
                json.append(quote(String.valueOf(entry.getKey())))
                    .append(':')
                    .append(toJson(entry.getValue()));
            }
            return json.append('}').toString();
        }
        if (value instanceof Iterable<?>) {
            StringBuilder json = new StringBuilder("[");
            boolean first = true;
            for (Object item : (Iterable<?>) value) {
                if (!first) {
                    json.append(',');
                }
                first = false;
                json.append(toJson(item));
            }
            return json.append(']').toString();
        }
        return quote(value.toString());
    }

    private static String quote(String value) {
        StringBuilder json = new StringBuilder("\"");
        for (int index = 0; index < value.length(); index++) {
            char character = value.charAt(index);
            switch (character) {
                case '"':
                    json.append("\\\"");
                    break;
                case '\\':
                    json.append("\\\\");
                    break;
                case '\b':
                    json.append("\\b");
                    break;
                case '\f':
                    json.append("\\f");
                    break;
                case '\n':
                    json.append("\\n");
                    break;
                case '\r':
                    json.append("\\r");
                    break;
                case '\t':
                    json.append("\\t");
                    break;
                default:
                    if (character < 0x20) {
                        json.append(String.format("\\u%04x", (int) character));
                    } else {
                        json.append(character);
                    }
            }
        }
        return json.append('"').toString();
    }

    public static void main(String[] args) {
        if (args.length != 1) {
            System.err.println("Usage: java CustomerLookup <customerusername>");
            System.exit(2);
        }

        String jdbcUrl = System.getenv("JDBC_URL");
        if (jdbcUrl == null || jdbcUrl.trim().isEmpty()) {
            System.err.println("JDBC_URL environment variable must be set.");
            System.exit(2);
        }

        Properties properties = new Properties();
        String user = System.getenv("JDBC_USER");
        String password = System.getenv("JDBC_PASSWORD");
        if (user != null) {
            properties.setProperty("user", user);
        }
        if (password != null) {
            properties.setProperty("password", password);
        }

        try (Connection connection = DriverManager.getConnection(jdbcUrl, properties)) {
            Customer customer = findCustomer(connection, args[0]);
            System.out.println(toJson(customer));
        } catch (SQLException exception) {
            System.err.println("Database error: " + exception.getMessage());
            System.exit(1);
        }
    }
}