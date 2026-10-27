import javax.crypto.Cipher;
import javax.crypto.SecretKey;
import javax.crypto.SecretKeyFactory;
import javax.crypto.spec.GCMParameterSpec;
import javax.crypto.spec.PBEKeySpec;
import javax.crypto.spec.SecretKeySpec;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;
import java.security.spec.InvalidKeySpecException;
import java.security.spec.KeySpec;
import java.time.Instant;
import java.util.Arrays;
import java.util.Base64;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;
import java.util.Optional;
import java.util.UUID;

public class SecureDataVault {

    private static final String ENCRYPTION_ALGORITHM = "AES/GCM/NoPadding";
    private static final String KEY_DERIVATION_ALGORITHM = "PBKDF2WithHmacSHA256";
    private static final int AES_KEY_SIZE_BITS = 256;
    private static final int GCM_TAG_LENGTH_BITS = 128;
    private static final int GCM_IV_LENGTH_BYTES = 12;
    private static final int PBKDF2_SALT_LENGTH_BYTES = 16;
    private static final int PBKDF2_ITERATIONS = 100_000;

    private final SecureRandom secureRandom;
    private final Map<String, VaultRecord> storage;

    public enum DataType {
        CREDIT_CARD,
        SSN,
        GENERIC_PII
    }

    public static final class VaultRecord {
        private final String recordId;
        private final DataType dataType;
        private final String maskedValue;
        private final byte[] salt;
        private final byte[] iv;
        private final byte[] cipherText;
        private final Instant createdAt;

        public VaultRecord(String recordId, DataType dataType, String maskedValue, byte[] salt, byte[] iv, byte[] cipherText, Instant createdAt) {
            this.recordId = recordId;
            this.dataType = dataType;
            this.maskedValue = maskedValue;
            this.salt = salt.clone();
            this.iv = iv.clone();
            this.cipherText = cipherText.clone();
            this.createdAt = createdAt;
        }

        public String getRecordId() {
            return recordId;
        }

        public DataType getDataType() {
            return dataType;
        }

        public String getMaskedValue() {
            return maskedValue;
        }

        public byte[] getSalt() {
            return salt.clone();
        }

        public byte[] getIv() {
            return iv.clone();
        }

        public byte[] getCipherText() {
            return cipherText.clone();
        }

        public Instant getCreatedAt() {
            return createdAt;
        }

        @Override
        public String toString() {
            return String.format("VaultRecord[ID=%s, Type=%s, Masked=%s, CreatedAt=%s]",
                    recordId, dataType, maskedValue, createdAt);
        }
    }

    public SecureDataVault() {
        this.secureRandom = new SecureRandom();
        this.storage = new HashMap<>();
    }

    public VaultRecord storeSensitiveData(char[] masterPassword, DataType dataType, String plainSensitiveData) {
        if (masterPassword == null || masterPassword.length == 0) {
            throw new IllegalArgumentException("Master password cannot be empty.");
        }
        if (plainSensitiveData == null || plainSensitiveData.isEmpty()) {
            throw new IllegalArgumentException("Sensitive data cannot be empty.");
        }

        String recordId = UUID.randomUUID().toString();
        byte[] salt = new byte[PBKDF2_SALT_LENGTH_BYTES];
        byte[] iv = new byte[GCM_IV_LENGTH_BYTES];
        this.secureRandom.nextBytes(salt);
        this.secureRandom.nextBytes(iv);

        SecretKey derivedKey = null;
        byte[] plainBytes = plainSensitiveData.getBytes(StandardCharsets.UTF_8);
        try {
            derivedKey = deriveKey(masterPassword, salt);
            Cipher cipher = Cipher.getInstance(ENCRYPTION_ALGORITHM);
            GCMParameterSpec gcmSpec = new GCMParameterSpec(GCM_TAG_LENGTH_BITS, iv);
            cipher.init(Cipher.ENCRYPT_MODE, derivedKey, gcmSpec);
            byte[] cipherText = cipher.doFinal(plainBytes);

            String masked = maskData(dataType, plainSensitiveData);

            VaultRecord record = new VaultRecord(
                    recordId,
                    dataType,
                    masked,
                    salt,
                    iv,
                    cipherText,
                    Instant.now()
            );

            this.storage.put(recordId, record);
            return record;
        } catch (Exception e) {
            throw new SecurityException("Failed to securely store data.", e);
        } finally {
            Arrays.fill(plainBytes, (byte) 0);
        }
    }

    public String retrieveSensitiveData(char[] masterPassword, String recordId) {
        if (masterPassword == null || masterPassword.length == 0) {
            throw new IllegalArgumentException("Master password cannot be empty.");
        }
        VaultRecord record = this.storage.get(recordId);
        if (record == null) {
            throw new IllegalArgumentException("Record ID not found: " + recordId);
        }

        SecretKey derivedKey = null;
        try {
            derivedKey = deriveKey(masterPassword, record.getSalt());
            Cipher cipher = Cipher.getInstance(ENCRYPTION_ALGORITHM);
            GCMParameterSpec gcmSpec = new GCMParameterSpec(GCM_TAG_LENGTH_BITS, record.getIv());
            cipher.init(Cipher.DECRYPT_MODE, derivedKey, gcmSpec);

            byte[] plainBytes = cipher.doFinal(record.getCipherText());
            String decrypted = new String(plainBytes, StandardCharsets.UTF_8);
            Arrays.fill(plainBytes, (byte) 0);
            return decrypted;
        } catch (Exception e) {
            throw new SecurityException("Decryption failed. Invalid credentials or corrupted data.", e);
        }
    }

    public Optional<VaultRecord> getRecordMetadata(String recordId) {
        return Optional.ofNullable(this.storage.get(recordId));
    }

    public Map<String, VaultRecord> getAllRecordsMetadata() {
        return Collections.unmodifiableMap(this.storage);
    }

    public boolean deleteRecord(String recordId) {
        return this.storage.remove(recordId) != null;
    }

    private SecretKey deriveKey(char[] password, byte[] salt)
            throws NoSuchAlgorithmException, InvalidKeySpecException {
        KeySpec spec = new PBEKeySpec(password, salt, PBKDF2_ITERATIONS, AES_KEY_SIZE_BITS);
        SecretKeyFactory factory = SecretKeyFactory.getInstance(KEY_DERIVATION_ALGORITHM);
        byte[] keyBytes = factory.generateSecret(spec).getEncoded();
        return new SecretKeySpec(keyBytes, "AES");
    }

    public static String maskData(DataType type, String rawValue) {
        if (rawValue == null || rawValue.isEmpty()) {
            return "";
        }
        switch (type) {
            case CREDIT_CARD:
                String sanitizedCard = rawValue.replaceAll("[^0-9]", "");
                if (sanitizedCard.length() < 4) {
                    return "****";
                }
                String lastFour = sanitizedCard.substring(sanitizedCard.length() - 4);
                return "****-****-****-" + lastFour;
            case SSN:
                String sanitizedSsn = rawValue.replaceAll("[^0-9]", "");
                if (sanitizedSsn.length() < 4) {
                    return "***-**-****";
                }
                return "***-**-" + sanitizedSsn.substring(sanitizedSsn.length() - 4);
            case GENERIC_PII:
            default:
                if (rawValue.length() <= 2) {
                    return "**";
                }
                return rawValue.charAt(0) + "***" + rawValue.charAt(rawValue.length() - 1);
        }
    }

    public static void main(String[] args) {
        SecureDataVault vault = new SecureDataVault();
        char[] masterPass = "MasterP@ssw0rd!2026".toCharArray();

        try {
            System.out.println("--- Secure Data Vault Demonstration ---\n");

            VaultRecord cardRecord = vault.storeSensitiveData(
                    masterPass,
                    DataType.CREDIT_CARD,
                    "4111-2222-3333-4444"
            );
            System.out.println("Stored Credit Card:");
            System.out.println("Record ID: " + cardRecord.getRecordId());
            System.out.println("Masked Value: " + cardRecord.getMaskedValue());
            System.out.println("Ciphertext (Base64): " + Base64.getEncoder().encodeToString(cardRecord.getCipherText()));
            System.out.println();

            VaultRecord ssnRecord = vault.storeSensitiveData(
                    masterPass,
                    DataType.SSN,
                    "000-12-3456"
            );
            System.out.println("Stored SSN:");
            System.out.println("Record ID: " + ssnRecord.getRecordId());
            System.out.println("Masked Value: " + ssnRecord.getMaskedValue());
            System.out.println("Ciphertext (Base64): " + Base64.getEncoder().encodeToString(ssnRecord.getCipherText()));
            System.out.println();

            System.out.println("--- Decryption with Correct Key ---");
            String decryptedCard = vault.retrieveSensitiveData(masterPass, cardRecord.getRecordId());
            System.out.println("Decrypted Card: " + decryptedCard);

            String decryptedSsn = vault.retrieveSensitiveData(masterPass, ssnRecord.getRecordId());
            System.out.println("Decrypted SSN: " + decryptedSsn);
            System.out.println();

            System.out.println("--- Decryption with Incorrect Key ---");
            char[] wrongPass = "WrongPassword#123".toCharArray();
            try {
                vault.retrieveSensitiveData(wrongPass, cardRecord.getRecordId());
            } catch (SecurityException e) {
                System.out.println("Expected error caught: " + e.getMessage());
            } finally {
                Arrays.fill(wrongPass, ' ');
            }

        } finally {
            Arrays.fill(masterPass, ' ');
        }
    }
}