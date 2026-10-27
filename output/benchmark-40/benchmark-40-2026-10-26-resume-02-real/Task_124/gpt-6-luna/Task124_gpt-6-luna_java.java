import javax.crypto.AEADBadTagException;
import javax.crypto.Cipher;
import javax.crypto.SecretKeyFactory;
import javax.crypto.spec.GCMParameterSpec;
import javax.crypto.spec.PBEKeySpec;
import javax.crypto.spec.SecretKeySpec;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;
import java.nio.charset.StandardCharsets;
import java.nio.file.AtomicMoveNotSupportedException;
import java.nio.file.Files;
import java.nio.file.LinkOption;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.nio.file.StandardOpenOption;
import java.nio.file.attribute.PosixFilePermission;
import java.nio.file.attribute.PosixFilePermissions;
import java.security.GeneralSecurityException;
import java.security.SecureRandom;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

public final class SecureDataStore implements AutoCloseable {
    private static final int MAGIC = 0x53445331;
    private static final byte VERSION = 1;
    private static final int SALT_LENGTH = 16;
    private static final int NONCE_LENGTH = 12;
    private static final int KEY_LENGTH_BITS = 256;
    private static final int PBKDF2_ITERATIONS = 310_000;
    private static final int TAG_LENGTH_BITS = 128;
    private static final int MAX_FILE_SIZE = 64 * 1024 * 1024;
    private static final int MAX_FIELD_SIZE = 4 * 1024 * 1024;
    private static final int MAX_ENTRIES = 100_000;

    private static final Set<PosixFilePermission> PRIVATE_FILE_PERMISSIONS =
            PosixFilePermissions.fromString("rw-------");

    private final Path path;
    private final SecureRandom secureRandom = new SecureRandom();

    private byte[] salt;
    private byte[] keyBytes;
    private Map<String, String> entries;
    private boolean closed;

    public SecureDataStore(Path path, char[] password)
            throws IOException, GeneralSecurityException {
        if (path == null) {
            throw new IllegalArgumentException("Path must not be null");
        }
        if (password == null || password.length < 12) {
            throw new IllegalArgumentException("Password must contain at least 12 characters");
        }

        this.path = path.toAbsolutePath().normalize();
        Path parent = this.path.getParent();
        if (parent == null) {
            throw new IllegalArgumentException("Store path must have a parent directory");
        }
        Files.createDirectories(parent);

        if (Files.isSymbolicLink(this.path)) {
            throw new IOException("Refusing to open a symbolic-link store");
        }

        char[] passwordCopy = password.clone();
        try {
            if (Files.exists(this.path, LinkOption.NOFOLLOW_LINKS)) {
                if (!Files.isRegularFile(this.path, LinkOption.NOFOLLOW_LINKS)) {
                    throw new IOException("Store path is not a regular file");
                }
                setPrivatePermissions(this.path);

                byte[] fileBytes = Files.readAllBytes(this.path);
                ParsedFile parsed = parseFile(fileBytes);
                this.salt = parsed.salt;
                this.keyBytes = deriveKey(passwordCopy, this.salt);
                this.entries = decryptEntries(parsed, this.keyBytes);
            } else {
                this.salt = new byte[SALT_LENGTH];
                secureRandom.nextBytes(this.salt);
                this.keyBytes = deriveKey(passwordCopy, this.salt);
                this.entries = new HashMap<>();
                persist(this.entries);
            }
        } catch (IOException | GeneralSecurityException | RuntimeException exception) {
            clearSecrets();
            throw exception;
        } finally {
            Arrays.fill(passwordCopy, '\0');
        }
    }

    public synchronized void put(String key, String value)
            throws IOException, GeneralSecurityException {
        ensureOpen();
        validateField(key, "key", false);
        validateField(value, "value", true);

        Map<String, String> updated = new HashMap<>(entries);
        updated.put(key, value);
        persist(updated);
        entries = updated;
    }

    public synchronized String get(String key) {
        ensureOpen();
        if (key == null) {
            throw new IllegalArgumentException("Key must not be null");
        }
        return entries.get(key);
    }

    public synchronized boolean contains(String key) {
        ensureOpen();
        if (key == null) {
            throw new IllegalArgumentException("Key must not be null");
        }
        return entries.containsKey(key);
    }

    public synchronized boolean delete(String key)
            throws IOException, GeneralSecurityException {
        ensureOpen();
        if (key == null) {
            throw new IllegalArgumentException("Key must not be null");
        }
        if (!entries.containsKey(key)) {
            return false;
        }

        Map<String, String> updated = new HashMap<>(entries);
        updated.remove(key);
        persist(updated);
        entries = updated;
        return true;
    }

    public synchronized List<String> keys() {
        ensureOpen();
        List<String> result = new ArrayList<>(entries.keySet());
        Collections.sort(result);
        return Collections.unmodifiableList(result);
    }

    @Override
    public synchronized void close() {
        if (!closed) {
            clearSecrets();
            entries = null;
            closed = true;
        }
    }

    private void persist(Map<String, String> values)
            throws IOException, GeneralSecurityException {
        byte[] plaintext = serializeEntries(values);
        byte[] nonce = new byte[NONCE_LENGTH];
        secureRandom.nextBytes(nonce);

        byte[] ciphertext;
        try {
            Cipher cipher = Cipher.getInstance("AES/GCM/NoPadding");
            cipher.init(Cipher.ENCRYPT_MODE, new SecretKeySpec(keyBytes, "AES"),
                    new GCMParameterSpec(TAG_LENGTH_BITS, nonce));
            cipher.updateAAD(createAad(salt));
            ciphertext = cipher.doFinal(plaintext);
        } finally {
            Arrays.fill(plaintext, (byte) 0);
        }

        byte[] fileBytes = encodeFile(salt, nonce, ciphertext);
        if (fileBytes.length > MAX_FILE_SIZE) {
            throw new IOException("Encrypted store exceeds the maximum supported size");
        }

        Path parent = path.getParent();
        Path temporary = createPrivateTemporaryFile(parent);
        try {
            try (FileChannel channel = FileChannel.open(
                    temporary, StandardOpenOption.WRITE, StandardOpenOption.TRUNCATE_EXISTING)) {
                ByteBuffer buffer = ByteBuffer.wrap(fileBytes);
                while (buffer.hasRemaining()) {
                    channel.write(buffer);
                }
                channel.force(true);
            }
            setPrivatePermissions(temporary);

            try {
                Files.move(temporary, path,
                        StandardCopyOption.ATOMIC_MOVE,
                        StandardCopyOption.REPLACE_EXISTING);
            } catch (AtomicMoveNotSupportedException exception) {
                Files.move(temporary, path, StandardCopyOption.REPLACE_EXISTING);
            }
            setPrivatePermissions(path);
        } finally {
            Files.deleteIfExists(temporary);
            Arrays.fill(fileBytes, (byte) 0);
            Arrays.fill(ciphertext, (byte) 0);
        }
    }

    private static byte[] serializeEntries(Map<String, String> values) throws IOException {
        if (values.size() > MAX_ENTRIES) {
            throw new IOException("Too many entries");
        }

        ByteArrayOutputStream byteStream = new ByteArrayOutputStream();
        try (DataOutputStream output = new DataOutputStream(byteStream)) {
            output.writeInt(values.size());
            for (Map.Entry<String, String> entry : values.entrySet()) {
                writeString(output, entry.getKey());
                writeString(output, entry.getValue());
                if (byteStream.size() > MAX_FILE_SIZE) {
                    throw new IOException("Store data exceeds the maximum supported size");
                }
            }
        }
        return byteStream.toByteArray();
    }

    private static void writeString(DataOutputStream output, String value) throws IOException {
        byte[] bytes = value.getBytes(StandardCharsets.UTF_8);
        if (bytes.length > MAX_FIELD_SIZE) {
            Arrays.fill(bytes, (byte) 0);
            throw new IOException("A field exceeds the maximum supported size");
        }
        output.writeInt(bytes.length);
        output.write(bytes);
        Arrays.fill(bytes, (byte) 0);
    }

    private static Map<String, String> decryptEntries(ParsedFile parsed, byte[] keyBytes)
            throws GeneralSecurityException, IOException {
        Cipher cipher = Cipher.getInstance("AES/GCM/NoPadding");
        cipher.init(Cipher.DECRYPT_MODE, new SecretKeySpec(keyBytes, "AES"),
                new GCMParameterSpec(TAG_LENGTH_BITS, parsed.nonce));
        cipher.updateAAD(createAad(parsed.salt));

        byte[]