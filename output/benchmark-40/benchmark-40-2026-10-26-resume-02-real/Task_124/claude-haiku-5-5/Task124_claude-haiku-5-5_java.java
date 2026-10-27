import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.security.GeneralSecurityException;
import java.security.SecureRandom;
import java.util.*;
import javax.crypto.Cipher;
import javax.crypto.SecretKeyFactory;
import javax.crypto.spec.GCMParameterSpec;
import javax.crypto.spec.PBEKeySpec;
import javax.crypto.spec.SecretKeySpec;

public class SensitiveDataVault {

    private static final String KDF_ALGORITHM = "PBKDF2WithHmacSHA256";
    private static final String CIPHER_ALGORITHM = "AES/GCM/NoPadding";
    private static final int KDF_ITERATIONS = 210000;
    private static final int KEY_BITS = 256;
    private static final int SALT_BYTES = 16;
    private static final int IV_BYTES = 12;
    private static final int TAG_BITS = 128;
    private static final String HEADER_PREFIX = "SALT:";
    private static final SecureRandom RANDOM = new SecureRandom();

    private final Path file;
    private final SecretKeySpec key;
    private final byte[] salt;
    private final Map<String, String> entries = new LinkedHashMap<>();

    public SensitiveDataVault(Path file, char[] password) throws GeneralSecurityException, IOException {
        this.file = file;
        if (Files.exists(file)) {
            List<String> lines = Files.readAllLines(file, StandardCharsets.UTF_8);
            if (lines.isEmpty() || !lines.get(0).startsWith(HEADER_PREFIX)) {
                throw new IOException("Corrupt vault: missing header");
            }
            this.salt = Base64.getDecoder().decode(lines.get(0).substring(HEADER_PREFIX.length()));
            this.key = deriveKey(password, salt);
            for (int i = 1; i < lines.size(); i++) {
                String line = lines.get(i);
                if (line.isBlank()) continue;
                int idx = line.indexOf(':');
                if (idx < 0) throw new IOException("Corrupt vault: bad entry on line " + (i + 1));
                entries.put(line.substring(0, idx), line.substring(idx + 1));
            }
            if (!entries.isEmpty()) {
                decrypt(entries.values().iterator().next(), key);
            }
        } else {
            this.salt = new byte[SALT_BYTES];
            RANDOM.nextBytes(this.salt);
            this.key = deriveKey(password, salt);
            save();
        }
    }

    public String addRecord(Map<String, String> fields) throws GeneralSecurityException, IOException {
        if (fields == null || fields.isEmpty()) {
            throw new IllegalArgumentException("No fields provided");
        }
        Map<String, String> clean = new TreeMap<>();
        for (Map.Entry<String, String> e : fields.entrySet()) {
            if (e.getKey() == null || e.getKey().isBlank()) continue;
            String value = e.getValue() == null ? "" : e.getValue().trim();
            if ("cardNumber".equals(e.getKey())) {
                value = value.replaceAll("[ -]", "");
                if (!passesLuhn(value)) {
                    throw new IllegalArgumentException("Invalid card number (failed validation)");
                }
            }
            clean.put(e.getKey().trim(), value);
        }
        String id = UUID.randomUUID().toString();
        entries.put(id, encrypt(serialize(clean), key));
        save();
        return id;
    }

    public Map<String, String> getRecord(String id) throws GeneralSecurityException {
        String token = entries.get(id);
        if (token == null) throw new NoSuchElementException("No record with id " + id);
        return deserialize(decrypt(token, key));
    }

    public Map<String, String> getMaskedRecord(String id) throws GeneralSecurityException {
        Map<String, String> record = getRecord(id);
        Map<String, String> masked = new TreeMap<>();
        for (Map.Entry<String, String> e : record.entrySet()) {
            String k = e.getKey();
            String v = e.getValue();
            if ("cardNumber".equalsIgnoreCase(k)) {
                masked.put(k, maskCard(v));
            } else if ("ssn".equalsIgnoreC